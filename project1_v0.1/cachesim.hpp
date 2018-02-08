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
    uint64_t prefetch_issued;
    uint64_t prefetch_hits;
    uint64_t prefetch_misses;
    uint64_t accesses_l2;
    uint64_t accesses_vc;
    uint64_t reads;
    uint64_t read_hits_l1;
    uint64_t read_misses_l1;
    uint64_t read_misses_l2;
    uint64_t writes;
    uint64_t write_hits_l1;
    uint64_t write_misses_l1;
    uint64_t write_misses_l2;
    uint64_t write_back_l1;
    uint64_t write_back_l2;
    uint64_t total_hits_l1;
    uint64_t total_misses_l1;
    double l1_hit_ratio;
    double l1_miss_ratio;
    double overall_miss_ratio;
    double read_hit_ratio;
    double read_miss_ratio;
    double write_hit_ratio;
    double write_miss_ratio;
    double prefetch_hit_ratio;
    uint64_t victim_hits;
    double   avg_access_time_l1;
};

struct config_t{
    uint64_t c;
    uint64_t b;
    uint64_t s;
    uint64_t p;
    uint64_t t;
};

struct cache{
    uint64_t *tag_store;
    uint64_t *timer;
    uint64_t *valid_bit;
    uint64_t *dirty_bit;
    config_t *config;
    cache *next;
};

struct prediction{
    uint64_t arg;
    uint64_t counter;
};

void setup_cache(config_t config, unsigned char ***data_store, uint64_t **tag_store, uint64_t **valid_bit, uint64_t **timer, uint64_t **dirty_bit);

void cache_access(char type, uint64_t arg, cache_stats_t* p_stats, config_t config, unsigned char **data_store, uint64_t *tag_store, uint64_t *valid_bit, uint64_t *timer, uint64_t time, uint64_t *dirty_bit); 
void complete_cache(cache_stats_t *p_stats, config_t config, unsigned char ***data_store, uint64_t **tag_store, uint64_t **timer, uint64_t **valid_bit, uint64_t **dirty_bit);

#define setbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] |= ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
#define clearbit(arr, pos) ( arr[pos/(sizeof(uint64_t)*CHAR_BIT)] &= ~(1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT))) )
#define testbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] & ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
#define getoffset(arg, config) ( arg & ((1UL<<config.b)-1) )
#define getindex(arg, config) ( (arg >> config.b) & ((1UL<<(config.c - config.b - config.s))-1) )
#define gettag(arg, index) ( (arg >> (config.c-config.s)) )

static const uint64_t DEFAULT_C1 = 12;   /* 4KB Cache */
static const uint64_t DEFAULT_B1 = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S1 = 3;    /* 8 blocks per set */
static const uint64_t DEFAULT_P1 = 0;    /* No prefetcher */
static const uint64_t DEFAULT_C2 = 15;   /* 32KB Cache */
static const uint64_t DEFAULT_B2 = 5;    /* 32-byte blocks */
static const uint64_t DEFAULT_S2 = 4;    /* 16 blocks per set */
static const uint64_t DEFAULT_V =  3;    /* 3 blocks in VC */
static const uint64_t DEFAULT_T =  0;    /* No prefetcher */
static const uint64_t MP = 20; 
static const uint64_t PBUFFER_SIZE = 32; 
static const uint64_t MARKOV_PREFETCHER_COLS = 4; 


/** Argument to cache_access rw. Indicates a load */
static const char     READ = 'r';
/** Argument to cache_access rw. Indicates a store */
static const char     WRITE = 'w';

#endif /* CACHESIM_HPP */
