#include "cachesim.hpp"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<limits.h>
#include<inttypes.h>

/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c1 The total number of bytes for data storage in L1 is 2^c
 * @b1 The size of L1's blocks in bytes: 2^b-byte blocks.
 * @s1 The number of blocks in each set of L1: 2^s blocks per set.
 */
void setup_cache(config_t *config, cache **L1_t, cache **L2_t, prefetcher_t *prefetcher, markov *markov_logic) {
    cache *L1 = (cache*)malloc(sizeof(cache));
    cache *L2 = (cache*)malloc(sizeof(cache));

    L1->config = config; L1->next = L2;
    config_t conf = (config_t*)malloc(sizeof(config_t));
    L2->config = conf;
    L2->config->c = DEFAULT_C2; L2->config->b = DEFAULT_B2; L2->config->s = DEFAULT_S2; L2->config->p = 0; L2->config->t = 0; L2->next = NULL;

    uint64_t num_blocks_L1 = 1UL<<(L1->config->c - L1->config->b);
    uint64_t num_blocks_L2 = 1UL<<(L2->config->c - L2->config->b);

    L1->tag_store = (uint64_t*)malloc(sizeof(uint64_t)*num_blocks_L1);
    memset(L1->tag_store, 0, sizeof(uint64_t)*num_blocks_L1);
    L2->tag_store = (uint64_t*)malloc(sizeof(uint64_t)*num_blocks_L2);
    memset(L2->tag_store, 0, sizeof(uint64_t)*num_blocks_L2);

    uint64_t bit_array_length_L1 = num_blocks_L1%(sizeof(uint64_t)*CHAR_BIT) == 0 ? num_blocks_L1/CHAR_BIT : sizeof(uint64_t);  
    uint64_t bit_array_length_L2 = num_blocks_L2%(sizeof(uint64_t)*CHAR_BIT) == 0 ? num_blocks_L2/CHAR_BIT : sizeof(uint64_t);  

    L1->valid_bit = (uint64_t*)malloc(bit_array_length_L1);
    memset(L1->valid_bit, 0, bit_array_length_L1);
    L2->valid_bit = (uint64_t*)malloc(bit_array_length_L2);
    memset(L2->valid_bit, 0, bit_array_length_L2);

    L1->timer = (uint64_t*)malloc(sizeof(uint64_t)*num_blocks_L1);
    memset(L1->timer, 0, sizeof(uint64_t)*num_blocks_L1);
    L2->timer = (uint64_t*)malloc(sizeof(uint64_t)*num_blocks_L2);
    memset(L2->timer, 0, sizeof(uint64_t)*num_blocks_L2);

    L1->dirty_bit = (uint64_t*)malloc(bit_array_length_L1);
    memset(L1->dirty_bit, 0, bit_array_length_L1);
    L2->dirty_bit = (uint64_t*)malloc(bit_array_length_L2);
    memset(L2->dirty_bit, 0, bit_array_length_L2);
    
    if(L1->config->t!=0){
        uint64_t *prefetch_buffer = (uint64_t*)malloc(sizeof(uint64_t)*PBUFFER_SIZE);
        memset(prefetch_buffer, 0, sizeof(uint64_t)*PBUFFER_SIZE);
        prefetcher->prefetch_buffer = prefetch_buffer;
        uint64_t *prefetch_buffer_dirty_bit = (uint64_t*)malloc(sizeof(uint64_t));
        memset(prefetch_buffer_dirty_bit, 0, sizeof(uint64_t));
        prefetcher->prefetch_buffer_dirty_bit = prefetch_buffer_dirty_bit;
        prefetcher->prefetch_buffer_size = 0;
        prefetcher->prev_miss = -1;
        if(config->t==1){
            uint64_t *markov_tag = (uint64_t*)malloc(sizeof(uint64_t)*L1->config->p);
            memset(markov_tag, 0, sizeof(uint64_t)*L1->config->p);
            markov_logic->tag = markov_tag;
            prediction **markov_matrix = (prediction**)malloc(sizeof(prediction*)*L1->config->p);
            for(size_t i=0; i<L1->config->p; i++)
                markov_matrix[i] = (prediction*)malloc(sizeof(prediction)*MARKOV_PREFETCHER_COLS);
            memset(markov_matrix, 0, L1->config->p*MARKOV_PREFETCHER_COLS*sizeof(prediction));
            markov_logic->matrix = markov_matrix;
            markov_logic->timer = (uint64_t*)malloc(L1->config->p*sizeof(uint64_t));
            memset(markov_logic->timer, 0, sizeof(uint64_t)*L1->config->p);
            markov_logic->row_size=0;
        }
    }
    *L1_t = L1; *L2_t = L2;
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @type The type of event, can be READ or WRITE.
 * @arg  The target memory address
 * @p_stats Pointer to the statistics structure
 */
int isPresentInCache(int toAdd, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t index = getindex(arg, c->config);
    uint64_t tag = gettag(arg, c->config);

    uint64_t block_start = index * (1UL << c->config->s);
    uint64_t block_end = block_start + (1UL << c->config->s) - 1;

    int flag=0;
    for(size_t i=block_start; i<=block_end; i++)
        if(testbit(c->valid_bit, i) && c->tag_store[i]==tag){
            flag=1;
            if(toAdd){
                c->timer[i] = time;
                p_stats->total_hits_l1++;
                if(type==READ) p_stats->read_hits_l1++;
                else { p_stats->write_hits_l1++; setbit(c->dirty_bit, i); }
            }
            break;
        }
    return flag;
}

int addToCache(int isDirty, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t index = getindex(arg, c->config);
    uint64_t tag = gettag(arg, c->config);

    uint64_t block_start = index * (1UL << c->config->s);
    uint64_t block_end = block_start + (1UL << c->config->s) - 1;

    int flag=0;
    for(size_t j=block_start; j<=block_end; j++)
        if(!testbit(c->valid_bit, j)){
            c->tag_store[j] = tag;
            setbit(c->valid_bit, j);
            if(type==READ && !isDirty) clearbit(c->dirty_bit, j);
            else setbit(c->dirty_bit, j);
            c->timer[j] = time;
            flag = 1; break;
        }
    return flag;
}

void addToFullCache(int isDirty, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t index = getindex(arg, c->config);
    uint64_t tag = gettag(arg, c->config);

    uint64_t block_start = index * (1UL << c->config->s);
    uint64_t block_end = block_start + (1UL << c->config->s) - 1;

    int flag=0, isDirty;
    uint64_t lru = UINT_MAX;
    size_t replace_pos;
    for(size_t j=block_start; j<=block_end; j++)
        if(c->timer[j]<lru){
            lru = c->timer[j];
            replace_pos = j;
        }
    uint64_t replace_arg = (c->tag_store[replace_pos] << (c->config->c - c->config->s)) | (index << c->config->b);
    if(c->next!=NULL){
        if(testbit(c->dirty_bit, replace_pos)) {p_stats->write_back_l1++; isDirty=1;}   
        else isDirty=0;
        if(!addToCache(isDirty, c->next, time, type, replace_arg, p_stats))
            addToFullCache(isDirty, c->next, time, type, replace_arg, p_stats); 
    }
    else{
        p_stats->accesses_l2++;
        if(testbit(c->dirty_bit, replace_pos)) p_stats->write_back_l2++;    
    }
    c->tag_store[replace_pos] = tag;
    setbit(c->valid_bit, replace_pos);
    if(type==READ && !isDirty) clearbit(c->dirty_bit, replace_pos);
    else setbit(c->dirty_bit, replace_pos);
    c->timer[replace_pos] = time; 
}

int removeFromCache(cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t tag = gettag(arg, c->config);
    uint64_t index = getindex(arg, c->config);
    uint64_t block_start = index * (1UL << c->config->s);
    uint64_t block_end = block_start + (1UL << c->config->s) - 1;

    int flag=0, isDirty=0;
    for(size_t i=block_start; i<=block_end; i++)
        if(testbit(c->valid_bit, i) && c->tag_store[i]==tag){
            if(testbit(c->dirty_bit, i)) isDirty = 1;
            clearbit(c->valid_bit, i);
            clearbit(c->dirty_bit, i);
            c->timer[i] = 0;
            p_stats->accesses_l2++;
            flag=1;
        }
    if(!flag)
        if(type==READ) p_stats->read_misses_l2++;
        else p_stats->write_misses_l2++;
    return isDirty;
}

int isPresentiInBuffer(prefetcher_t *prefetcher, int toRemove, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t arg_t = arg >> c->config->b;
    int flag=0, isDirty;
    for(size_t i=0; i<prefetcher->prefetch_buffer_size; i++)
        if(arg_t==prefetcher->prefetch_buffer[i]){
            flag=1;
            if(toRemove){
                p_stats->prefetch_hits++;
                if(testbit(prefetcher->prefetch_buffer_dirty_bit, i)) isDirty = 1;
                else isDirty = 0;
                if(!addToCache(isDirty, c, time, type, arg, p_stats))
                    addToFullCache(isDirty, c, time, type, arg, p_stats);
                //remove from prefetch buffer
                for(size_t j=i+1; j<prefetcher->prefetch_buffer_size; j++)
                    prefetcher->prefetch_buffer[j-1] = prefetcher->prefetch_buffer[j];
                prefetcher->prefetch_buffer_size--;
            }
            break;
        }
    if(!flag) p_stats->prefetch_misses++;
    return flag;
}

void prefetch(prefetcher_t *prefetcher, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t p_stats){
    int isDirty;
    p_stats->prefetch_issued++; 
    if(prefetcher->prefetch_buffer_size==PBUFFER_SIZE){
        buf_remove = prefetcher->prefetch_buffer[0] << c->config->b;
        if(testbit(prefetcher->prefetch_buffer_dirty_bit, 0)) isDirty=1;
        else isDirty=0;
        if(!addToCache(isDirty, c->next, time, type, buf_remove, p_stats))
            addToFullCache(isDirty, c->next, time, type, buf_remove, p_stats);
        for(size_t i=1; i<prefetch_buffer_size; i++)
            prefetch_buffer[i-1] = prefetch_buffer[i];
        prefetcher->prefetch_buffer[prefetcher->prefetch_buffer_size - 1] = arg;
    }
    else{
        prefetcher->prefetch_buffer[prefetcher->prefetch_buffer_size] = arg;
        prefetcher->prefetch_buffer_size++;
    }
    int isDirty = removeFromCache(c->next, time, type, arg, p_stats);
    if(isDirty) setbit(prefetcher->prefetch_buffer_dirty_bit, prefetcher->prefetch_buffer_size-1);
}

int markov_prefetcher(markov *markov_logic, prefetcher_t *prefetcher, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t p_stats){
    uint64_t arg_t = arg >> c->config->b;
    int ifPrefetched = 0;
    for(size_t i=0; i<markov_logic->row_size; i++)
       if(arg_t==markov_logic->tag[i]){
           prediction mfu; mfu.counter = -1; mfu.arg = 0;
           int pos;
           for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
                if(markov_logic->matrix[i][j].counter > mfu.counter){
                    mfu = markov_logic->matrix[i][j];
                    pos = j;
                }
                else if((markov_logic->matrix[i][j].counter == mfu.counter) && (markov_logic->matrix[i][j].arg > mfu.arg)){
                    mfu = markov_logic->matrix[i][j];
                    pos = j;
                }
           arg_to_prefetch = mfu.arg<<(c->config->b);
           if(arg_to_prefetch!=arg_t && !isPresentInCache(0, c, time, type, arg_to_prefetch, p_stats) && !isPresentInBuffer(prefetcher, 0, c, time, type, arg_to_prefetch, p_stats))
               prefetch(prefetcher, c, time, type, arg_to_prefetch, p_stats);
           ifPrefetched = 1;
       }           
    int flag=0;
    for(size_t i=0; i<markov_logic->row_size; i++)
        if(prefetcher->prev_miss == markov_logic->tag[i]){
            markov_logic->timer[i] = time;
            for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
                if(markov_logic->matrix[i][j].arg == arg_t){
                   markov_logic->matrix[i][j].counter++;
                   flag=1;
                }
            if(!flag){
                for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
                    if(markov_logic->matrix[i][j].counter==0){
                        markov_logic->matrix[i][j].arg=arg_t; markov_logic->matrix[i][j].counter = 1;
                        flag=1;
                    }
                if(!flag){
                    prediction lfu; lfu.arg = -1; lfu.counter = UINT_MAX;
                    int replace_pos;
                    for(size_t j=0; j< MARKOV_PREFETCHER_COLS; j++)
                        if(markov_logic->matrix[i][j].counter < lfu.counter){
                            lfu = markov_logic->matrix[i][j];
                            replace_pos = j;
                        }
                        else if((markov_logic->matrix[i][j].counter==lfu.counter) && (markov_logic->matrix[i][j]arg < lfu.arg)){
                            lfu = markov_matrix[i][j];
                            replace_pos = j;
                        }
                    lfu.arg=arg_t; lfu.counter=1;
                    markov_logic->matrix[i][replace_pos] = lfu;
                }
            }
            flag=1;
        }
    if(!flag){
        int replace_pos;
        if(markov_logic->row_size == c->config->p){
            uint64_t lru = UINT_MAX; 
            for(size_t i=0; i<c->config->p; i++)
                if(markov_logic->timer[i]<lru){
                    lru = markov_logic->timer[i];
                    replace_pos = i;
                }
            markov_matrix->timer[replace_pos] = time;
            markov_matrix->tag[replace_pos] = prefetcher->prev_miss;
            memset(markov_logic->matrix[replace_pos], 0, MARKOV_PREFETCHER_COLS*sizeof(prediction));
        }
        else{
            markov_matrix->timer[row_size] = time;
            markov_matrix->tag[row_size] = prefetcher->prev_miss;
            memset(markov_logic->matrix[row_size], 0, MARKOV_PREFETCHER_COLS*sizeof(prediction));
            replace_pos = row_size;
            markov_matrix->row_size++;
        }
        for(size_t i=0; i<MARKOV_PREFETCHER_COLS; i++)
            if(markov_logic->matrix[replace_pos][i].counter==0){
                markov_logic->matrix[replace_pos][i].arg = arg_t;
                markov_logic->matrix[replace_pos][i].counter = 1;
            }
    }

    int isDirty = removeFromCache(c->next, time, type, arg, p_stats);
    if(!addToCache(isDirty, c, time, type, arg, p_stats))
        addToFullCache(isDirty, c, time, type, arg, p_stats);
    prefetcher->prev_miss = arg_t;
    return ifPrefetched;
}

void sequential_prefetcher(prefetcher_t *prefetcher, cache *c, uint64_t time, char type. uint64_t arg, cache_stats_t *p_stats){
    uint64_t index = getindex(arg, c->config);
    uint64_t tag = gettag(arg, c->config);
    uint64_t arg_to_prefetch = (tag << (c->config->c - c->config->s)) | ((index+1) << c->config->b);
    if(arg_to_prefetch!=arg_t && !isPresentInCache(0, c, time, type, arg_to_prefetch, p_stats) && !isPresentInBuffer(prefetcher, 0, c, time, type, arg_to_prefetch, p_stats))
        prefetch(prefetcher, c, time, type, arg_to_prefetch, p_stats);

    int isDirty = removeFromCache(c->next, time, type, arg, p_stats);
    if(!addToCache(isDirty, c, time, type, arg, p_stats))
        addToFullCache(isDirty, c, time, type, arg, p_stats);
    prefetcher->prev_miss = arg_t;
}   

void hybrid_prefetcher(markov *markov_logic, prefetcher_t *prefetcher, cache *c, uint64_t time, char type. uint64_t arg, cache_stats_t *p_stats){
    if(!markov_prefetcher(markov_logic, prefetcher, L1, time, type, arg, p_stats)){
        uint64_t index = getindex(arg, c->config);
        uint64_t tag = gettag(arg, c->config);
        uint64_t arg_to_prefetch = (tag << (c->config->c - c->config->s)) | ((index+1) << c->config->b);

        if(arg_to_prefetch!=arg_t && !isPresentInCache(0, c, time, type, arg_to_prefetch, p_stats) && !isPresentInBuffer(prefetcher, 0, c, time, type, arg_to_prefetch, p_stats))
            prefetch(prefetcher, c, time, type, arg_to_prefetch, p_stats);
    }
}


void cache_access(char type, uint64_t arg, cache_stats_t* p_stats, cache *L1, uint64_t time, prefetcher_t *prefetcher, markov *markov_logic) {
    /*
    uint64_t num_sets = 1UL<<(config->c-config->b-config->s);
    uint64_t num_blocks = 1UL<<(config->c-config->b);
    uint64_t associativity = 1UL<<config->s;
    */
    /* 
    printf("Type : %c\n", type); 
    printf("Tag : %" PRIu64 "\n", tag);
    printf("Index : %" PRIu64 "\n", index);
    printf("Set : %" PRIu64 "\n", set);
    printf("Offset : %" PRIu64 "\n", offset);
    */

    //L1 cache check
    if(!isPresentiInCache(1, L1, time, type, arg, p_stats){
        if(type==READ) p_stats->read_misses_l1;
        else p_stats->write_misses_l1;
        //check buffer
        if(!isPresentiInBuffer(prefetcher, 1, L1, time, type, arg, p_stats)){
            //Markov prefetcher
            if(L1->config->t==1)
                markov_prefetcher(markov_logic, prefetcher, L1, time, type, arg, p_stats);
            else if(L1->config->t==2)
                sequential_prefetcher(prefetcher, L1, time, type, arg, p_stats);
            else if (L1->config->t==3)
                hybrid_prefetcher();
            else{
                
            }
        }
}

void no_prefetcher_cache_access(uint64_t block_start, uint64_t block_end, uint64_t *valid_bit, uint64_t *dirty_bit, uint64_t *timer, uint64_t time, uint64_t *tag_store, cache_stats_t* p_stat) {
    int flag=0;
    if(type == READ){
        for(size_t i=block_start; i<=block_end; i++){
            if( testbit(valid_bit, i) && (tag_store[i]==tag) ){
                p_stats->read_hits_l1++;
                p_stats->total_hits_l1++;
                timer[i] = time;
                flag=1; break;
            }
        }
        if(flag) printf("H\n");
        if(!flag){
            printf("M\n");
            p_stats->read_misses_l1++;
            p_stats->total_misses_l1++;
            // create a isSetFull bit to check if the set is full and a isCacheFull bit
            for(size_t i=block_start; i<=block_end; i++)
                if(!testbit(valid_bit, i)){
                    tag_store[i] = tag;
                    setbit(valid_bit, i);
                    clearbit(dirty_bit, i);
                    timer[i] = time; 
                    flag = 1; break;
                }
            if(!flag){
                uint64_t lru = UINT_MAX;
                size_t replace_pos;
                for(size_t i=block_start; i<=block_end; i++){
                    if(timer[i]<lru){
                        lru = timer[i];
                        replace_pos = i;
                    }
                }
                if(testbit(dirty_bit, replace_pos))
                    ++p_stats->write_back_l1;
                tag_store[replace_pos] = tag;
                setbit(valid_bit, replace_pos);
                clearbit(dirty_bit, replace_pos);
                timer[replace_pos] = time; 
            }
        }
    } 
    else{
        flag=0;
        for(size_t i=block_start; i<=block_end; i++){
            if( testbit(valid_bit, i) && (tag_store[i]==tag) ){
                p_stats->write_hits_l1++;
                p_stats->total_hits_l1++;
                setbit(dirty_bit, i);
                timer[i] = time;
                flag = 1; break;
            }
        }
        if(flag) printf("H\n");
        if(!flag){
            printf("M\n");
            p_stats->write_misses_l1++;
            p_stats->total_misses_l1++;
            for(size_t i=block_start; i<=block_end; i++)
                if(!testbit(valid_bit, i)){
                    tag_store[i] = tag;
                    setbit(valid_bit, i);
                    setbit(dirty_bit, i);
                    timer[i] = time; 
                    flag = 1; break;
                }
            if(!flag){
                uint64_t lru = UINT_MAX;
                size_t replace_pos;
                for(size_t i=block_start; i<=block_end; i++){
                    if(timer[i]<lru){
                        lru = timer[i];
                        replace_pos = i;
                    }
                }
                if(testbit(dirty_bit, replace_pos)) 
                    ++p_stats->write_back_l1;
                tag_store[replace_pos] = tag;
                setbit(valid_bit, replace_pos);
                setbit(dirty_bit, replace_pos);
                timer[replace_pos] = time; 
            }
        }
    }
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats, config_t config, unsigned char ***data_store, uint64_t **tag_store, uint64_t **timer, uint64_t **valid_bit, uint64_t **dirty_bit) {
    p_stats->total_hit_ratio = (double)p_stats->total_hits_l1/(double)p_stats->accesses;    
    p_stats->total_miss_ratio = (double)p_stats->total_misses_l1/(double)p_stats->accesses;    

    p_stats->read_hit_ratio = (double)p_stats->read_hits_l1/(double)p_stats->reads;    
    p_stats->read_miss_ratio = (double)p_stats->read_misses_l1/(double)p_stats->reads;    

    p_stats->write_hit_ratio = (double)p_stats->write_hits_l1/(double)p_stats->writes;    
    p_stats->write_miss_ratio = (double)p_stats->write_misses_l1/(double)p_stats->writes;    
    
    p_stats->avg_access_time_l1 = (2+(0.2*config->s)) + (20*p_stats->total_miss_ratio);
    
    unsigned char **data_store_t = *data_store;
    for(size_t i=0; i<(1UL<<(config->c - config->b)); i++){
        free(data_store_t[i]);
        data_store_t[i] = NULL;
    }
    free(data_store_t);
    free(*tag_store);
    free(*timer);
    free(*valid_bit);
    free(*dirty_bit);
    data_store_t = NULL; *tag_store = NULL; *timer = NULL, *valid_bit = NULL; *dirty_bit = NULL;
}
