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
void setup_cache(cache *L1, cache *L2, uint64_t **prefetch_buffer_t, uint64_t **markov_tag_t, prediction ***markov_matrix_t) {
    uint64_t num_blocks_L1 = 1UL<<(L1->config.c - L1->config.b);
    uint64_t num_blocks_L2 = 1UL<<(L2->config.c - L2->config.b);

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
    
    if(config.t!=0){
        uint64_t *prefetch_buffer = (uint64_t*)malloc(sizeof(uint64_t)*PBUFFER_SIZE);
        memset(prefetch_buffer, 0, sizeof(uint64_t)*PBUFFER_SIZE);
        *prefetch_buffer_t = prefetch_buffer;
        if(config.t==1){
            uint64_t *markov_tag = (uint64_t*)malloc(sizeof(uint64_t)*config.p);
            memset(markov_tag, 0, sizeof(uint64_t)*config.p);
            *markov_tag_t = markov_tag;
            prediction **markov_matrix = (prediction**)malloc(sizeof(prediction*)*config.p);
            for(size_t i=0; i<config.p; i++)
                markov_matrix[i] = (prediction*)malloc(sizeof(prediction)*MARKOV_PREFETCHER_COLS);
            memset(markov_matrix, 0, sizeof(config.p*MARKOV_PREFETCHER_COLS*sizeof(prediction));
            *markov_matrix_t = markov_matrix);
        }
    }
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @type The type of event, can be READ or WRITE.
 * @arg  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char type, uint64_t arg, cache_stats_t* p_stats, cache *L1, cache *L2, uint64_t time, uint64_t *prefetch_buffer, uint64_t *markov_tag, prediction **markov_matrix) {
    uint64_t offset = arg & ((1UL<<config.b)-1);
    uint64_t index = (arg >> config.b) & ((1UL<<(config.c - config.b - config.s))-1);
    uint64_t tag = (arg >> (config.c-config.s));

    uint64_t num_sets = 1UL<<(config.c-config.b-config.s);
    uint64_t num_blocks = 1UL<<(config.c-config.b);
    uint64_t associativity = 1UL<<config.s;
    
    uint64_t block_start = index*(1UL<< config.s);
    uint64_t block_end = block_start + (1<<config.s) - 1; 

    /* 
    printf("Type : %c\n", type); 
    printf("Tag : %" PRIu64 "\n", tag);
    printf("Index : %" PRIu64 "\n", index);
    printf("Set : %" PRIu64 "\n", set);
    printf("Offset : %" PRIu64 "\n", offset);
    */
    
    if(config.t==0)
        no_prefetcher_cache_access(block_start, block_end, valid_bit, dirty_bit, timer, time, tag_store, p_stats);
    else if(config.t==1)
        markov_prefetcher_cache_access(block_start, block_end, valid_bit, dirty_bit, timer, time, tag_store, p_stats, prefetch_buffer, markov_tag, markov_matrix);
    else if(config.t==2)
    else
}

void markov_prefetcher_cache_access(uint64_t block_start, uint64_t block_end, uint64_t *valid_bit, uint64_t *dirty_bit, uint64_t *timer, uint64_t time, uint64_t *tag_store, cache_stats_t* p_stat, uint64_t *prefetch_buffer, uint64_t *markov_tag, prediction **markov_matrix) {


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
    
    p_stats->avg_access_time_l1 = (2+(0.2*config.s)) + (20*p_stats->total_miss_ratio);
    
    unsigned char **data_store_t = *data_store;
    for(size_t i=0; i<(1UL<<(config.c - config.b)); i++){
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
