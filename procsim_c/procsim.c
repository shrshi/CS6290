#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "procsim.h"

bool isFull(void *q, uint64_t *head, uint64_t *tail, uint64_t len)
{
    if((*head==0 && *tail==(len-1)) || (*head==*tail+1)){
        printf("Circular queue is full\n");
        //exit(0);
        return true;
    }
    return false;
}

void circular_enqueue(void *data, void *q, uint64_t *head, uint64_t *tail, uint64_t len)
{
    if(*tail==-1 && *head==-1)
        *head = 0;
    if(!isFull(q, head, tail, len)){
        if(*tail==(len-1)) *tail = -1;
        q[++*tail] = *data;
    }
}
       
       
/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 * @ROB Number of ROB Entries
 * @PREG Number of registers in the PRF
 */
void setup_proc(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t rob, uint64_t preg) 
{
    proc_params proc_settings;
    proc_settings.k0 = k0; proc_settings.k1 = k1; proc_settings.k2 = k2; proc_settings.f = f; proc_settings.rob = rob; proc_settings.preg = preg;
    
    inflight_q *inflight_queue = malloc(sizeof(inflight_q));
    inflight_queue->ifq = malloc(sizeof(proc_inst_t)*proc_settings.rob);
    inflight_queue->head = inflight_queue->tail = -1;

    uint64_t num_regs = 8*proc_settings.f;
    uint64_t num_bits = num_regs%(sizeof(uint64_t)*CHAR_BIT)!=0 ? sizeof(uint64_t) : num_regs/CHAR_BIT; //will not work if not power of 2
    regs *registers = malloc(sizeof(regs));
    registers->ready_bit = malloc(num_bits);
    memset(registers->ready_bit, 0, num_bits);
    for(int i=0; i<ARF; i++)
        setbit(registers->ready_bit, i);
    registers->head = malloc(sizeof(free_pregs)); registers->head->prev = NULL;
    free_pregs *temp = registers->head, *t;
    for(int i=ARF; i<num_regs-1; i++){
        temp->number = i;
        t = temp;
        temp->next = malloc(sizeof(free_pregs));
        temp = temp->next;
        temp->prev = t;
    }
    temp->number = num_regs-1;
    temp->next = NULL;
    registers->tail = temp;
    registers->num_free_pregs = num_regs - ARF;

    uint64_t *rat = malloc(sizeof(uint64_t)*ARF); 
    for(int i=0; i<ARF; i++)
        rat[i] = i;
   
    sched_q *schedule_queue = malloc(sizeof(sched_q));
    schedule_queue->num_entries = 0;
    schedule_queue->head->prev = NULL; schedule_queue->head->next = NULL; schedule_queue->tail->prev = NULL; schedule_queue->tail->next = NULL; 

    rob_queue *r_queue = malloc(sizeof(rob_queue));
    r_queue->rob = malloc(sizeof(rob_entry)*proc_settings.rob);
    num_bits = proc_settings.rob%(sizeof(uint64_t)*CHAR_BIT)!=0 ? sizeof(uint64_t) : proc_settings.rob/CHAR_BIT; //will not work if not power of 2
    r_queue->ready_bit = malloc(num_bits);
    memset(r_queue->ready_bit, 0, num_bits);
    r_queue->exception_bit = malloc(num_bits);
    memset(r_queue->exception_bit, 0, num_bits);
    r_queue->head = r_queue->tail = -1;
    
    proc_inst_t **scoreboard = malloc(sizeof(proc_inst_t*)*(proc_settings.k0+proc_settings.k1+proc_settings.k2));
    memset(scoreboard, 0, sizeof(proc_inst_t*)*(proc_settings.k0+proc_settings.k1+proc_settings.k2));
}

int dispatch(proc_inst_t instance)
{
    int flag = 1;
    if(schedule_queue->num_entries < proc_settings.rob && !isFull(r_queue->rob, r_queue->head, r_queue->tail, proc_settings.rob) && registers->num_free_pregs > 0){
        flag=0;
        sched_q_entry *temp = malloc(sizeof(sched_q_entry));
        rob_entry r;
        
        temp->instr = &instance;
        instance.sqe = &temp;
        r->instr = &instance;
        instance.re = &r;
        temp->fu = instance.op_code>0?instance.op_code:1;
        for(int i=0; i<2; i++)
            temp->src_preg[i] = rat[instance.src_reg[i]];
        r.dest_areg = instance.dest_reg;
        r.prev_preg = rat[instance.dest_reg];
        rat[instance.dest_reg] = registers->head->number; free_pregs *t = registers->head; registers->head = registers->head->next; registers->head->prev = NULL; free(t); registers->num_free_pregs--;
        temp->dest_preg = rat[instance.dest_reg];
        clearbit(registers->ready_bit, rat[instance.dest_reg]);
        circular_enqueue(&r, r_queue->rob, &r_queue->head, &r_queue->tail, proc_settings.rob);
        clearbit(r_queue->ready_bit, r_queue->tail);
        clearbit(r_queue->exception_bit, r_queue->tail);

        temp->next = NULL;
        if(schedule_queue->head->next==NULL){
            schedule_queue->head->next = temp;
            temp->prev = schedule_queue->head;
        }
        else{
            schedule_queue->tail->next = temp;
            temp->prev = schedule_queue->tail;
        }
        schedule_queue->tail = temp;
    }
    return flag;
}


void fire()
{
    int pos, f0, f1;
    sched_q_entry *t = schedule_queue->head;
    while(t!=NULL){
        f0=0; f1=0;
        for(size_t i=r_queue->head; i<r_queue->tail; i++){
            if(t->src_preg[0]==rat[r_queue->rob[i]->dest_areg] && testbit(r_queue->ready_bit, i)) f0=1;
            if(t->src_preg[1]==rat[r_queue->rob[i]->dest_areg] && testbit(r_queue->ready_bit, i)) f1=1;
            if(i==proc_settings.rob-1) i=0;
        }
        if((testbit(registers->ready_bit, t->src_preg[0]) || f0) && (testbit(registers->ready_bit, t->src_preg[1]) || f1)){
            int offset = (t->fu==2 ? (proc_settings.k0+proc_settings.k1) : (t->fu==1 ? proc_settings.k0 : 0));
            int flag=1;
            for(int i=offset; i<(t->fu==2 ? (proc_settings.k0+proc_settings.k1+proc_settings.k2) : (t->fu==1 ? (proc_settings.k0+proc_settings.k1) : proc_settings.k0)); i++)
                if(scoreboard[i]==NULL){
                    flag=0;
                    scoreboard[i] = t->instr;
                    (*t->instr)->scoreboarde = &scoreboard[i];
                    break;
                }
        }
        t = t->next;
    }
}

void complete()
{
    rob_entry r;
    for(int i=0; i<(proc_settings.k0+proc_settings.k1+proc_settings.k2); i++){
        if(scoreboard[i]!=NULL){
            r = (*scoreboard[i])->re;
            for(size_t i=r_queue->head; i<r_queue->tail; i++){
                if(t->src_preg[0]==rat[r_queue->rob[i]->dest_areg] && testbit(r_queue->ready_bit, i)) f0=1;
                if(t->src_preg[1]==rat[r_queue->rob[i]->dest_areg] && testbit(r_queue->ready_bit, i)) f1=1;
                if(i==proc_settings.rob-1) i=0;
            }
            scoreboard[i]=NULL;
        }
    }
}

/**
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void run_proc(proc_stats_t* p_stats)
{
    uint64_t cycles = 1, time=1;
    while(1){
        proc_inst_t instance;
        for(int i=0; i<proc_settings.f; i++){
            if(!read_instruction(&instance)) {printf("Couldn't read from file\n"); exit(0);} 
            instance.time = time;
            instance.cycle = cycle;
            instance.scoreboarde = NULL; instance.sqe = NULL; instance.re = NULL;
            time++;
            if(!dispatch(instance))
                circular_enqueue(&instance, inflight_queue->ifq, &inflight_queue->head, &inflight_queue->tail, proc_settings.rob);
            else break;
        }
        fire();
        complete();
        cycle++;
    }
}

/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats) 
{

}
