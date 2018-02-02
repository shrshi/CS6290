#include "cachesim.hpp"

/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c1 The total number of bytes for data storage in L1 is 2^c
 * @b1 The size of L1's blocks in bytes: 2^b-byte blocks.
 * @s1 The number of blocks in each set of L1: 2^s blocks per set.
 */
void setup_cache(config_t config, unsigned char **data_store, uint64_t *tag_store, uint64_t *valid_bit, uint64_t *timer, uint64_t *dirty_bit) {
    uint64_t num_blocks = 1<<(config.c-config.b);
    data_store = malloc(sizeof(unsigned char*)*num_blocks);
    for(size_t i=0; i<num_blocks; i++)
        data_store[i] = malloc(sizeof(unsigned char)*(1<<config.b)); 
    tag_store = malloc(sizeof(uint64_t)*num_blocks);
    valid_bit = malloc((sizeof(uint64_t))*(num_blocks/sizeof(uint64_t));
    memset(valid_bit, 0, sizeof(uint64_t)*(num_blocks/sizeof(uint64_t)));
    uint64_t num_sets = (1<<(config.c - config.b - config.s));
    timer = malloc(sizeof(uint64_t)*num_blocks);
    memset(timer, 0, sizeof(uint64_t)*num_blocks);
    dirty_bit = malloc((sizeof(uint64_t))*(num_blocks/sizeof(uint64_t));
    memset(dirty_bit, sizeof(uint64_t)*(num_blocks/sizeof(uint64_t)));
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
    uint64_t offset = arg & ((1<<config.b)-1);
    uint64_t index = (arg >> config.b) & ((1<<(config.c - config.b - config.s))-1);
    uint64_t tag = (arg >> (config.c-config.s)) & ((1<<(64 - config.c + config.s))-1);
    
    uint64_t set = index*(1<< config.s);
    int flag=0;
    if(strcmp(type,READ)==0){
        for(size_t i=set; i<(set+(1<<config.s)); i++)
            if( (valid_bit[i/sizeof(uint64_t)] & ( 1<<(i%sizeof(uint64_t)) )) && (tag_store[i]==tag) ){
                p_stats->read_hits_l1++;
                p_stats->total_hits_l1++;
                timer[i] = time;
                flag=1;
            }
        if(!flag){
            p_stats->read_misses_l1++;
            p_stat->total_misses_l1++;
            flag = 0;
            for(size_t i=set; i<(set+(1<<config.s)); i++)
                if(!(valid_bit[i/sizeof(uint64_t)] & ( 1<<(i%sizeof(uint64_t)) )) ){
                    tag_store[i] = tag;
                    valid_bit[i/sizeof(uint64_t)] |= 1<<(i%sizeof(uint64_t));
                    timer[i] = time; 
                    flag = 1;
                }
            if(!flag){
                uint64_t lru = 1<<64;
                uint64_t replace_pos;
                for(size_t i=set; i<(set+(1<<config.s)); i++)
                    if(timer[i]<lru){
                        lru = timer[i];
                        replace_pos = i;
                    }
                tag_store[replace_pos] = tag;
                valid_bit[replace_pos/sizeof(uint64_t)] |= 1<<(replace_pos%sizeof(uint64_t));
                timer[i] = time; 
            }
        }
    } 
    else{
        for(size_t i=set; i<(set+(1<<config.s)); i++)

    }
}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats) {

}
