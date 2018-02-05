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
void setup_cache(config_t config, unsigned char ***data_store_t, uint64_t **tag_store_t, uint64_t **valid_bit_t, uint64_t **timer_t, uint64_t **dirty_bit_t) {
    uint64_t num_blocks = 1UL<<(config.c-config.b);
    unsigned char **data_store; uint64_t *tag_store, *valid_bit, *timer, *dirty_bit;

    data_store = (unsigned char**)malloc(sizeof(unsigned char*)*num_blocks);
    for(size_t i=0; i<num_blocks; i++)
        data_store[i] = (unsigned char*)malloc(sizeof(unsigned char)*(1UL<<config.b)); 

    tag_store = (uint64_t*)malloc(sizeof(uint64_t)*num_blocks);
    memset(tag_store, 0, sizeof(uint64_t)*num_blocks);

    uint64_t bit_array_length = num_blocks%(sizeof(uint64_t)*CHAR_BIT) == 0 ? num_blocks/CHAR_BIT : sizeof(uint64_t);  

    valid_bit = (uint64_t*)malloc(bit_array_length);
    memset(valid_bit, 0, bit_array_length);

    timer = (uint64_t*)malloc(sizeof(uint64_t)*num_blocks);
    memset(timer, 0, sizeof(uint64_t)*num_blocks);

    dirty_bit = (uint64_t*)malloc(bit_array_length);
    memset(dirty_bit, 0, bit_array_length);

    *data_store_t = data_store; *tag_store_t = tag_store; *valid_bit_t = valid_bit; *timer_t = timer; *dirty_bit_t = dirty_bit;
}

/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @type The type of event, can be READ or WRITE.
 * @arg  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char type, uint64_t arg, cache_stats_t* p_stats, config_t config, unsigned char **data_store, uint64_t *tag_store, uint64_t *valid_bit, uint64_t *timer, uint64_t time, uint64_t *dirty_bit) {
    uint64_t offset = arg & ((1UL<<config.b)-1);
    uint64_t index = (arg >> config.b) & ((1UL<<(config.c - config.b - config.s))-1);
    uint64_t tag = (arg >> (config.c-config.s));

    uint64_t set = index*(1UL<< config.s);
    
    uint64_t num_sets = 1UL<<(config.c-config.b-config.s);
    uint64_t associativity = 1UL<<config.s;
    /* 
    printf("Type : %c\n", type); 
    printf("Tag : %" PRIu64 "\n", tag);
    printf("Index : %" PRIu64 "\n", index);
    printf("Set : %" PRIu64 "\n", set);
    printf("Offset : %" PRIu64 "\n", offset);
    */
    uint64_t block_start = set;
    uint64_t block_end = set+(1<<config.s)-1;

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
