#ifndef PROCSIM_H
#define PROCSIM_H

#define DEFAULT_K0 3
#define DEFAULT_K1 2
#define DEFAULT_K2 1
#define DEFAULT_ROB 12
#define DEFAULT_F 4
#define DEFAULT_PREG 32
#define ARF 32


typedef struct _proc_params
{
    uint64_t f;
    uint64_t k0;
    uint64_t k1;
    uint64_t k2;
    uint64_t rob;
    uint64_t preg;
} proc_params;

typedef struct _proc_inst_t
{
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
    
    // You may introduce other fields as needed
    
} proc_inst_t;

typedef struct _regs
{
    uint64_t *ready_bit;
    uint64_t *free_bit;
    uint64_t free_index;
} regs;

typedef struct _sched_q_entry
{
    int fu;
    uint64_t dest_preg;
    uint64_t src_preg[2];
} sched_q_entry;

typedef struct _sched_q
{
    sched_q_entry *table;
    sched_q_entry *ptr;
} sched_q;

typedef struct _rob_entry
{
    int32_t dest_areg;
    uint64_t prev_preg;
    uint64_t pc_resume;
} rob_entry;

typedef struct _rob_queue
{
    rob_entry *head;
    rob_entry *tail;
    uint64_t *ready_bit;
    uint64_t *exception_bit;
    rob_entry *rob;
} rob_queue;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    unsigned long retired_instruction;
    unsigned long cycle_count;
    
} proc_stats_t;

extern regs *registers;
extern uint64_t *rat;
extern sched_q *schedule_queue;  
extern rob_queue *r_queue;
extern uint64_t *scoreboard;

#define setbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] |= ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
#define clearbit(arr, pos) ( arr[pos/(sizeof(uint64_t)*CHAR_BIT)] &= ~(1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT))) )
#define testbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] & ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
bool read_instruction(proc_inst_t* p_inst);
void setup_proc(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t rob, uint64_t preg);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

#endif /* PROCSIM_H */
