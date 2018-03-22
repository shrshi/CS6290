#ifndef PROCSIM_H
#define PROCSIM_H

#define DEFAULT_K0 3
#define DEFAULT_K1 2
#define DEFAULT_K2 1
#define DEFAULT_ROB 12
#define DEFAULT_F 4
#define DEFAULT_PREG 32
#define ARF 32
#define SENTINEL -1

struct _free_pregs;
struct _sched_q_entry;
struct _rob_entry;
struct _proc_inst_t;
//rob and inflight queue are circular queues; free_pregs and schedule queue are doubly linked lists
typedef struct _free_pregs
{
    uint64_t number;
    struct _free_pregs *next;
    struct _free_pregs *prev;
} free_pregs;

typedef struct _sched_q_entry
{
    int fu;
    uint64_t dest_preg;
    uint64_t src_preg[2];
    struct _proc_inst_t *instr;
    struct _sched_q_entry *next;
    struct _sched_q_entry *prev;    
} sched_q_entry;

typedef struct _rob_entry
{
    int32_t dest_areg;
    uint64_t prev_preg; //for preg freeing
    uint64_t pc_resume;
    struct _proc_inst_t *instr;
} rob_entry;

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
    uint64_t number;
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
    uint64_t dispatch_cycle;
    uint64_t fire_cycle;
    uint64_t complete_cycle;
    uint64_t retire_cycle;
    sched_q_entry *sqe;
    rob_entry *re;
    int scoreboarde;
} proc_inst_t;

typedef struct _inflight_q
{
    proc_inst_t *ifq;
    uint64_t head;
    uint64_t tail;
    int isEmpty;
} inflight_q;

typedef struct _regs
{
    uint64_t *ready_bit;
    free_pregs *head;
    free_pregs *tail;
    uint64_t num_free_pregs;
} regs;

typedef struct _sched_q
{
    sched_q_entry *head;
    sched_q_entry *tail;
    uint64_t num_entries;
} sched_q;


typedef struct _rob_queue
{
    uint64_t head;
    uint64_t tail;
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

extern proc_params proc_settings;
extern inflight_q *inflight_queue;
extern regs *registers;
extern uint64_t *rat;
extern sched_q *schedule_queue;  
extern rob_queue *r_queue;
extern proc_inst_t **scoreboard;

#define setbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] |= ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
#define clearbit(arr, pos) ( arr[pos/(sizeof(uint64_t)*CHAR_BIT)] &= ~(1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT))) )
#define testbit(arr, pos) (arr[pos/(sizeof(uint64_t)*CHAR_BIT)] & ( 1UL<<(pos%(sizeof(uint64_t)*CHAR_BIT)) ))
bool read_instruction(proc_inst_t* p_inst);
void setup_proc(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t rob, uint64_t preg);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);

bool isFull(uint64_t head, uint64_t tail, uint64_t len);
void circular_enqueue(uint64_t *head, uint64_t *tail, uint64_t len);
void circular_dequeue(uint64_t *head, uint64_t *tail, uint64_t len);
void dispatch(proc_inst_t *instance);
void fire(uint64_t cycle);
void complete(uint64_t cycle);
void retire(uint64_t cycle);
#endif /* PROCSIM_H */
