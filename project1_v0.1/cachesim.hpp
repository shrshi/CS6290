#ifndef CACHESIM_HPP
#define CACHESIM_HPP

#ifdef CCOMPILER
#include<stdint.h>
#include<inttypes.h>
#include<limits.h>
#else
#include <cstdint>
#endif

struct cache_stats_t {
    uint64_t accesses;
    uint64_t reads;
    uint64_t read_hits_l1;
    uint64_t read_misses_l1;
    uint64_t writes;
    uint64_t write_hits_l1;
    uint64_t write_misses_l1;
    uint64_t write_back_l1;
    uint64_t total_hits_l1;
    uint64_t total_misses_l1;
    double total_hit_ratio;
    double total_miss_ratio;
    double read_hit_ratio;
    double read_miss_ratio;
    double write_hit_ratio;
    double write_miss_ratio;
    double avg_access_time_l1;
};

struct config_t{
    uint64_t c;
    uint64_t b;
    uint64_t s;
};

void setup_cache(config_t config, unsigned char ***data_store, uint64_t **tag_store, uint64_t **valid_bit, uint64_t **timer, uint64_t **dirty_bit);

void cache_access(char type, uint64_t arg, cache_stats_t* p_stats, config_t config, unsigned char **data_store, uint64_t *tag_store, uint64_t *valid_bit, uint64_t *timer, uint64_t time, uint64_t *dirty_bit); 
void complete_cache(cache_stats_t *p_stats, config_t config, unsigned char ***data_store, uint64_t **tag_store, uint64_t **timer, uint64_t **valid_bit, uint64_t **dirty_bit);

#define setbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] |= ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
#define clearbit(arr, pos) ( arr[pos/(sizeof(uint64_t)*CHAR_BIT)] &= ~(1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT))) )
#define testbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] & ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))

static const uint64_t DEFAULT_C1 = 12;   /* 4KB Cache */
static const uint64_t DEFAULT_B1 = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S1 = 3;    /* 8 blocks per set */

/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_HPP */
