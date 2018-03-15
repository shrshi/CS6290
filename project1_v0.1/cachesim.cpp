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
void setup_cache(config_t *config, cache **L1_t, cache **L2_t, prefetcher_t **prefetcher, markov **markov_logic) {
    cache *L1 = (cache*)malloc(sizeof(cache));
    cache *L2 = (cache*)malloc(sizeof(cache));

    L1->config = config; L1->next = L2;
    config_t *conf = (config_t*)malloc(sizeof(config_t));
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
        *prefetcher = (prefetcher_t*)malloc(sizeof(prefetcher_t));

        (*prefetcher)->prefetch_buffer = (uint64_t*)malloc(sizeof(uint64_t)*PBUFFER_SIZE);
        memset((*prefetcher)->prefetch_buffer, 0, sizeof(uint64_t)*PBUFFER_SIZE);

        (*prefetcher)->prefetch_buffer_dirty_bit = (uint64_t*)malloc(sizeof(uint64_t));
        memset((*prefetcher)->prefetch_buffer_dirty_bit, 0, sizeof(uint64_t));

        (*prefetcher)->prefetch_buffer_size = 0;
        (*prefetcher)->prev_miss = 0;

        if(L1->config->t!=2){
            *markov_logic = (markov*)malloc(sizeof(markov));

            (*markov_logic)->tag = (uint64_t*)malloc(sizeof(uint64_t)*L1->config->p);
            memset((*markov_logic)->tag, 0, sizeof(uint64_t)*L1->config->p);

            (*markov_logic)->matrix = (prediction **) malloc(sizeof(prediction *) * L1->config->p);
            for(size_t i=0; i<L1->config->p; i++)
                (*markov_logic)->matrix[i] = (prediction *) calloc(MARKOV_PREFETCHER_COLS, sizeof(prediction));
           
            (*markov_logic)->timer = (uint64_t*)malloc(L1->config->p*sizeof(uint64_t));
            memset((*markov_logic)->timer, 0, sizeof(uint64_t)*L1->config->p);

            (*markov_logic)->row_size=0;
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
    /* 
    printf("Type : %c\n", type); 
    printf("Tag : %" PRIu64 "\n", tag);
    printf("Index : %" PRIu64 "\n", index);
    printf("Block start : %"PRIu64"\n", block_start);
    printf("Block end : %"PRIu64"\n", block_end);
    */
    int flag=0;
    for(size_t i=block_start; i<=block_end; i++)
        if(testbit(c->valid_bit, i) && c->tag_store[i]==tag){
            flag=1;
            if(toAdd){
                c->timer[i] = time;
                p_stats->total_hits_l1++;
                printf("H\n");
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
            setbit(c->dirty_bit, j);
            if(c->next!=NULL && type==READ) clearbit(c->dirty_bit, j); //since there is no L2 cache :(
            if(c->next==NULL && !isDirty) clearbit(c->dirty_bit, j);
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

    int isDirty_t;
    uint64_t lru = UINT_MAX;
    size_t replace_pos;
    for(size_t j=block_start; j<=block_end; j++)
        if(c->timer[j]<lru){
            lru = c->timer[j];
            replace_pos = j;
        }
    uint64_t replace_arg = (c->tag_store[replace_pos] << (c->config->c - c->config->s)) | (index << c->config->b);
    if(c->next!=NULL){
        if(testbit(c->dirty_bit, replace_pos)) {p_stats->write_back_l1++; isDirty_t=1;}   
        else isDirty_t=0;
        if(!addToCache(isDirty_t, c->next, time, type, replace_arg, p_stats))
            addToFullCache(isDirty_t, c->next, time, type, replace_arg, p_stats); 
    }
    else{
        p_stats->accesses_l2++;
        if(testbit(c->dirty_bit, replace_pos)) p_stats->write_back_l2++;    
    }
    c->tag_store[replace_pos] = tag;
    setbit(c->valid_bit, replace_pos);
    setbit(c->dirty_bit, replace_pos);
    if(c->next!=NULL && type==READ) clearbit(c->dirty_bit, replace_pos);
    if(c->next==NULL && !isDirty) clearbit(c->dirty_bit, replace_pos);
    c->timer[replace_pos] = time; 
}

int removeFromCache(cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t tag = gettag(arg, c->config);
    uint64_t index = getindex(arg, c->config);
    uint64_t block_start = index * (1UL << c->config->s);
    uint64_t block_end = block_start + (1UL << c->config->s) - 1;
    /* 
    printf("In L2\n");
    printf("Type : %c\n", type); 
    printf("Tag : %" PRIu64 "\n", tag);
    printf("Index : %" PRIu64 "\n", index);
    printf("Block start : %"PRIu64"\n", block_start);
    printf("Block end : %"PRIu64"\n", block_end);
    */
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
    if(!flag){
        if(type==READ) p_stats->read_misses_l2++;
        else p_stats->write_misses_l2++;
    }
    return isDirty;
}

int isPresentInBuffer(prefetcher_t *prefetcher, int toRemove, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    uint64_t arg_t = arg & ~((1UL << c->config->b)-1);
    int flag=0, isDirty;
    for(int i=0; i<prefetcher->prefetch_buffer_size; i++){
        if(arg_t==prefetcher->prefetch_buffer[i]){
            flag=1;
            if(toRemove){
                p_stats->prefetch_hits++;
                printf("PH\n");
                if(testbit(prefetcher->prefetch_buffer_dirty_bit, i)) isDirty = 1;
                else isDirty = 0;
                if(!addToCache(isDirty, c, time, type, arg, p_stats))
                    addToFullCache(isDirty, c, time, type, arg, p_stats);
                //remove from prefetch buffer
                for(int j=i+1; j<prefetcher->prefetch_buffer_size; j++)
                    prefetcher->prefetch_buffer[j-1] = prefetcher->prefetch_buffer[j];
                prefetcher->prefetch_buffer_size--;
            }
            break;
        }
    }
    /* 
    printf("Arg to check in prefetch buffer and markov matrix : %"PRIu64"\n", arg_t);
    printf("Prefetch buffer : [");
    for(int i=0; i<prefetcher->prefetch_buffer_size; i++)
        printf("%"PRIu64", ", prefetcher->prefetch_buffer[i]);
    printf("]\n");
    */
    return flag;
}

void prefetch(prefetcher_t *prefetcher, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    int isDirty;
    p_stats->prefetch_issued++; 

    if(prefetcher->prefetch_buffer_size == PBUFFER_SIZE){
        uint64_t buf_remove = prefetcher->prefetch_buffer[0];
        if(testbit(prefetcher->prefetch_buffer_dirty_bit, 0)) isDirty=1;
        else isDirty=0;
        if(!addToCache(isDirty, c->next, time, type, buf_remove, p_stats))
            addToFullCache(isDirty, c->next, time, type, buf_remove, p_stats);
        for(int i=1; i<prefetcher->prefetch_buffer_size; i++)
            prefetcher->prefetch_buffer[i-1] = prefetcher->prefetch_buffer[i];
        prefetcher->prefetch_buffer[prefetcher->prefetch_buffer_size - 1] = arg;
    }
    else{
        prefetcher->prefetch_buffer[prefetcher->prefetch_buffer_size] = arg;
        ++prefetcher->prefetch_buffer_size;
    }
    uint64_t arg_to_remove = arg;
    isDirty = removeFromCache(c->next, time, type, arg_to_remove, p_stats);
    if(isDirty) setbit(prefetcher->prefetch_buffer_dirty_bit, (uint64_t)(prefetcher->prefetch_buffer_size - 1));
}

int markov_prefetcher(int fromHybrid, markov *markov_logic, prefetcher_t *prefetcher, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    //arg_t = X
    //arg_to_prefetch = C
    //markov_logic->prev_miss = Y
    uint64_t arg_t = arg & ~((1UL << c->config->b)-1);
    uint64_t arg_to_prefetch;
    int ifPrefetched = 0;
    for(int i=0; i<markov_logic->row_size; i++)
       if(arg_t==markov_logic->tag[i]){
           prediction mfu; mfu.counter = 0; mfu.arg = 0;
           for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
                if(markov_logic->matrix[i][j].counter > mfu.counter) memcpy(&mfu, &markov_logic->matrix[i][j], sizeof(prediction));
                else if((markov_logic->matrix[i][j].counter == mfu.counter) && (markov_logic->matrix[i][j].arg > mfu.arg)) memcpy(&mfu, &markov_logic->matrix[i][j], sizeof(prediction));

           arg_to_prefetch = mfu.arg;
           if(arg_to_prefetch!=arg_t && !isPresentInCache(0, c, time, type, arg_to_prefetch, p_stats) && !isPresentInBuffer(prefetcher, 0, c, time, type, arg_to_prefetch, p_stats)){
               //printf("Issue prefetch for addr: %"PRIx64"\n", arg_to_prefetch);
               prefetch(prefetcher, c, time, type, arg_to_prefetch, p_stats);
           }
           ifPrefetched = 1;
       }           
    int flag=0;
    if(time>1)
        //printf("Prev Miss Addr without offset: %"PRIx64"\n", prefetcher->prev_miss);
    //check if Y exists
    for(int i=0; i<markov_logic->row_size; i++)
        if(prefetcher->prev_miss == markov_logic->tag[i]){
            //printf("Row hit\n");
            markov_logic->timer[i] = time;
            for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
                if(markov_logic->matrix[i][j].arg == arg_t){
                   markov_logic->matrix[i][j].counter++;
                   flag=1; 
                   //printf("Column hit\n");
                   break;
                }
            if(!flag){
                //printf("Column miss\n");
                for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
                    if(markov_logic->matrix[i][j].counter==0){
                        markov_logic->matrix[i][j].arg=arg_t; markov_logic->matrix[i][j].counter = 1;
                        flag=1; break;
                    }
                if(!flag){
                    prediction lfu; lfu.arg = 0; lfu.counter = UINT_MAX;
                    int replace_pos;
                    for(size_t j=0; j< MARKOV_PREFETCHER_COLS; j++)
                        if(markov_logic->matrix[i][j].counter < lfu.counter){
                            memcpy(&lfu, &markov_logic->matrix[i][j], sizeof(prediction));
                            replace_pos = j;
                        }
                        else if((markov_logic->matrix[i][j].counter==lfu.counter) && (markov_logic->matrix[i][j].arg < lfu.arg)){
                            memcpy(&lfu, &markov_logic->matrix[i][j], sizeof(prediction));
                            replace_pos = j;
                        }
                    lfu.arg=arg_t; lfu.counter=1;
                    memcpy(&markov_logic->matrix[i][replace_pos], &lfu, sizeof(prediction));
                }
            }
            flag=1; break;
        }
    //if Y does not exist
    if(!flag && time>1){
        //printf("Row miss\n");
        //printf("Column miss\n");
        int replace_pos;
        if(markov_logic->row_size == c->config->p){
            uint64_t lru = UINT_MAX; 
            for(size_t i=0; i<c->config->p; i++)
                if(markov_logic->timer[i]<lru){
                    lru = markov_logic->timer[i];
                    replace_pos = i;
                }
            markov_logic->timer[replace_pos] = time;
            markov_logic->tag[replace_pos] = prefetcher->prev_miss;
            memset(markov_logic->matrix[replace_pos], 0, MARKOV_PREFETCHER_COLS*sizeof(prediction));
        }
        else{
            markov_logic->timer[markov_logic->row_size] = time;
            markov_logic->tag[markov_logic->row_size] = prefetcher->prev_miss;
            memset(markov_logic->matrix[markov_logic->row_size], 0, MARKOV_PREFETCHER_COLS*sizeof(prediction));
            replace_pos = markov_logic->row_size;
            markov_logic->row_size++;
        }
        for(size_t i=0; i<MARKOV_PREFETCHER_COLS; i++)
            if(markov_logic->matrix[replace_pos][i].counter==0){
                markov_logic->matrix[replace_pos][i].arg = arg_t;
                markov_logic->matrix[replace_pos][i].counter = 1;
                break;
            }
    }
    if(!fromHybrid){
        int isDirty = removeFromCache(c->next, time, type, arg, p_stats);
        if(!addToCache(isDirty, c, time, type, arg, p_stats))
            addToFullCache(isDirty, c, time, type, arg, p_stats);
    }
    prefetcher->prev_miss = arg_t;
    
    /* 
    printf("Markov predictor\n");
    for(size_t i=0; i<markov_logic->row_size; i++){
        printf("[%"PRIu64"]  |  ", markov_logic->tag[i]);
        for(size_t j=0; j<MARKOV_PREFETCHER_COLS; j++)
            printf("(%"PRIu64", %"PRIu64")  ", markov_logic->matrix[i][j].arg, markov_logic->matrix[i][j].counter);
        printf("\n");
    }
    */
    return ifPrefetched;
}

void sequential_prefetcher(prefetcher_t *prefetcher, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    //printf("In sequential predictor\n");
    uint64_t arg_t = arg & ~((1UL << c->config->b)-1);
    uint64_t arg_to_prefetch = arg_t + (1UL << c->config->b);
    if(!isPresentInCache(0, c, time, type, arg_to_prefetch, p_stats) && !isPresentInBuffer(prefetcher, 0, c, time, type, arg_to_prefetch, p_stats))
        prefetch(prefetcher, c, time, type, arg_to_prefetch, p_stats);

    int isDirty = removeFromCache(c->next, time, type, arg, p_stats);
    if(!addToCache(isDirty, c, time, type, arg, p_stats))
        addToFullCache(isDirty, c, time, type, arg, p_stats);
    prefetcher->prev_miss = arg_t;
}   

void hybrid_prefetcher(markov *markov_logic, prefetcher_t *prefetcher, cache *c, uint64_t time, char type, uint64_t arg, cache_stats_t *p_stats){
    if(!markov_prefetcher(1, markov_logic, prefetcher, c, time, type, arg, p_stats)){
        //printf("In sequential predictor\n");
        uint64_t arg_t = arg & ~((1UL << c->config->b)-1);
        uint64_t arg_to_prefetch = arg_t + (1UL << c->config->b);
        if(!isPresentInCache(0, c, time, type, arg_to_prefetch, p_stats) && !isPresentInBuffer(prefetcher, 0, c, time, type, arg_to_prefetch, p_stats))
            prefetch(prefetcher, c, time, type, arg_to_prefetch, p_stats);
    }
    int isDirty = removeFromCache(c->next, time, type, arg, p_stats);
    if(!addToCache(isDirty, c, time, type, arg, p_stats))
        addToFullCache(isDirty, c, time, type, arg, p_stats);
}


void cache_access(char type, uint64_t arg, cache_stats_t* p_stats, cache *L1, uint64_t time, prefetcher_t *prefetcher, markov *markov_logic) {
    //L1 cache check
    //printf("Curr Addr without offset: %"PRIx64"\n", arg & ~((1UL << L1->config->b)-1));
    if(!isPresentInCache(1, L1, time, type, arg, p_stats)){
        if(type==READ) p_stats->read_misses_l1++;
        else p_stats->write_misses_l1++;
        p_stats->total_misses_l1++;
        if(L1->config->t!=0){
            //check buffer
            if(!isPresentInBuffer(prefetcher, 1, L1, time, type, arg, p_stats)){
                printf("M\n");
                p_stats->prefetch_misses++;
                if(L1->config->t==1)
                    markov_prefetcher(0, markov_logic, prefetcher, L1, time, type, arg, p_stats);
                else if(L1->config->t==2)
                    sequential_prefetcher(prefetcher, L1, time, type, arg, p_stats);
                else if (L1->config->t==3)
                    hybrid_prefetcher(markov_logic, prefetcher, L1, time, type, arg, p_stats);
            }
        }
        else{
            printf("M\n");
            int isDirty = removeFromCache(L1->next, time, type, arg, p_stats);
            if(!addToCache(isDirty, L1, time, type, arg, p_stats))
                addToFullCache(isDirty, L1, time, type, arg, p_stats);
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
void complete_cache(cache_stats_t *p_stats, cache **L1_t, cache **L2_t, prefetcher_t **prefetcher, markov **markov_logic_t){
    p_stats->l1_hit_ratio = (double)p_stats->total_hits_l1/(double)p_stats->accesses;
    p_stats->l1_miss_ratio = (double)p_stats->total_misses_l1/(double)p_stats->accesses;
    //double l2_miss_ratio = (double)(p_stats->read_misses_l2 + p_stats->write_misses_l2)/(double)p_stats->accesses_l2;

    p_stats->read_hit_ratio = (double)p_stats->read_hits_l1/(double)p_stats->reads;
    p_stats->read_miss_ratio = (double)p_stats->read_misses_l1/(double)p_stats->reads;
    p_stats->write_hit_ratio = (double)p_stats->write_hits_l1/(double)p_stats->writes;
    p_stats->write_miss_ratio = (double)p_stats->write_misses_l1/(double)p_stats->writes;

    if(p_stats->prefetch_issued!=0) p_stats->prefetch_hit_ratio = (double)p_stats->prefetch_hits/(double)p_stats->prefetch_issued;
    else p_stats->prefetch_hit_ratio = 0.000;

    cache *L1 = *L1_t;
    if(L1->config->t!=0){
        //if L2 cache were there,    p_stats->overall_miss_ratio = ((double)p_stats->prefetch_misses/(double)p_stats->accesses)*l2_miss_ratio;
        p_stats->overall_miss_ratio = ((double)p_stats->prefetch_misses/(double)p_stats->accesses);
    }
    else{
        //if L2 cache were there,    p_stats->overall_miss_ratio = p_stats->l1_miss_ratio * l2_miss_ratio; 
        p_stats->overall_miss_ratio = p_stats->l1_miss_ratio; 
    }
    
    //if L2 cache were there,      p_stats->avg_access_time_l1 = (2 + 0.2*L1->config->s) + p_stats->l1_miss_ratio * ((2 + 0.2*L2->config->s) + l2_miss_ratio*20);
    p_stats->avg_access_time_l1 = (2.0 + (0.2*(double)(L1->config->s))) + (p_stats->overall_miss_ratio * 20); 
    if(L1->config->t!=0){
        free((*prefetcher)->prefetch_buffer);
        free((*prefetcher)->prefetch_buffer_dirty_bit);
        free(*prefetcher);
    
        if(L1->config->t!=2){
            markov *markov_logic = *markov_logic_t;
            for(size_t i=0; i<L1->config->p; i++)
                if(markov_logic->matrix[i]!=NULL)
                    free(markov_logic->matrix[i]);
            free(markov_logic->matrix);
            free(markov_logic->timer);
            free(markov_logic->tag);
            free(markov_logic);
        }
    }
    free(L1->tag_store);
    free(L1->timer);
    free(L1->valid_bit);
    free(L1->dirty_bit);
    free(L1->next->tag_store);
    free(L1->next->timer);
    free(L1->next->valid_bit);
    free(L1->next->dirty_bit);
    free(L1->next->config);
    free(L1->next);
    free(L1);
}
