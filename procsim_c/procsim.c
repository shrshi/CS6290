#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <inttypes.h>
#include "procsim.h"

bool isFull(uint64_t head, uint64_t tail, uint64_t len)
{
    if((head==0 && tail==(len-1)) || (head!=UINT_MAX && tail!=UINT_MAX && head==tail+1)){
        //printf("Circular queue is full\n");
        //exit(0);
        return true;
    }
    return false;
}

void circular_enqueue(uint64_t *head, uint64_t *tail, uint64_t len)
{
    if(*tail==UINT_MAX && *head==UINT_MAX)
        *head = *tail = 0;
    else if(!isFull(*head, *tail, len)){
        if(*tail==(len-1)) *tail = 0;
        else ++(*tail);
    }
}

void circular_dequeue(uint64_t *head, uint64_t *tail, uint64_t len)
{
    if(*head==UINT_MAX) return;
    if(*head==*tail){
        *head = UINT_MAX;
        *tail = UINT_MAX;
    }
    else if(*head==len-1)
        *head=0;
    else ++(*head);
}

void print_instr(proc_inst_t *instr)
{
    printf("INSTRUCTION\n%"PRIu32" %d %d %d %d\n", instr->instruction_address, instr->op_code, instr->src_reg[0], instr->src_reg[1], instr->dest_reg);
    printf("Dispatch cycle : %"PRIu64" ; Fire cycle : %"PRIu64" ; Complete cycle : %"PRIu64" ; Retire cycle : %"PRIu64" ; Scoreboard : %d\n", instr->dispatch_cycle, instr->fire_cycle, instr->complete_cycle, instr->retire_cycle, instr->scoreboarde);
}

void print_sched_queue()
{
    sched_q_entry *t = schedule_queue->head;
    printf("\n===================================SCHEDULING QUEUE=====================================\n");
    while(t!=NULL)
    {
        printf("%"PRIu64" | %"PRIu32" | %d | %"PRIu64" | %"PRIu64" | %"PRIu64"\n", t->instr->number, t->instr->instruction_address, t->fu, t->src_preg[0], t->src_preg[1], t->dest_preg);
        t = t->next;
    }
}

void print_rob_queue()
{
    rob_entry *r;
    printf("========================================= ROB =========================================\n");
    printf("Head : %"PRIu64", Tail : %"PRIu64"\n", r_queue->head, r_queue->tail);
    /*
    for(size_t i=r_queue->head; ; i++){
        r = &(r_queue->rob[i]);
        printf("%d | %"PRIu32" | %d | %"PRIu64"\n", r->instr->number, r->instr->instruction_address, r->dest_areg, r->prev_preg);
        if(i==proc_settings.rob-1 && r_queue->tail<r_queue->head) i=0;
        if(i==r_queue->tail) break;
    }
    */
    for(size_t i=0; i<proc_settings.rob; i++){
        r = &(r_queue->rob[i]);
        if(r->instr) printf("%"PRIu64" | %"PRIu32" | %d | %"PRIu64" | %"PRIu64" |  %d\n", r->instr->number, r->instr->instruction_address, r->dest_areg, r->prev_preg, r->curr_preg, testbit(r_queue->ready_bit, i)?1:0);
    }
}
       
void print_inflight_queue()
{
    proc_inst_t *instr;
    printf("\nINFLIGHT QUEUE\n");
    printf("Head : %"PRIu64", Tail : %"PRIu64"\n", inflight_queue->head, inflight_queue->tail);
    /*
    for(size_t i=inflight_queue->head; ; i++){
        instr = &(inflight_queue->ifq[i]);
        printf("%d | %"PRIu32" | %d | %d | %d | %d\n", instr->number, instr->instruction_address, instr->op_code, instr->src_reg[0], instr->src_reg[1], instr->dest_reg);
        printf("Dispatch cycle : %"PRIu64" ; Fire cycle : %"PRIu64" ; Complete cycle : %"PRIu64" ; Retire cycle : %"PRIu64" ; Scoreboard : %d\n", instr->dispatch_cycle, instr->fire_cycle, instr->complete_cycle, instr->retire_cycle, instr->scoreboarde);
        if(i==proc_settings.rob-1 && inflight_queue->tail<inflight_queue->head) i=0;
        if(i==inflight_queue->tail) break;
    }
    */
    for(size_t i=0; i<proc_settings.rob; i++){
        instr = &(inflight_queue->ifq[i]);
        if(instr){
            printf("%"PRIu64" | %"PRIu32" | %d | %d | %d | %d\n", instr->number, instr->instruction_address, instr->op_code, instr->src_reg[0], instr->src_reg[1], instr->dest_reg);
            printf("Dispatch cycle : %"PRIu64" ; Fire cycle : %"PRIu64" ; Complete cycle : %"PRIu64" ; Retire cycle : %"PRIu64" ; Scoreboard : %d\n", instr->dispatch_cycle, instr->fire_cycle, instr->complete_cycle, instr->retire_cycle, instr->scoreboarde);
        }
    }
}

void print_rat()
{
    printf("======================================== RAT ==========================================\n");
    for(int i=1; i<=ARF; i++){
        printf("%"PRIu64" ",rat[i-1]);
        if(i%(ARF/4)==0) printf("\n");
    }
}

void print_scoreboard()
{
    printf("\nSCOREBOARD\n");
    for(size_t i=0; i<(proc_settings.k0+proc_settings.k1+proc_settings.k2); i++)
        printf("%d ",(scoreboard[i]==NULL)?0:1);
    printf("\n");
}

proc_params proc_settings;
inflight_q *inflight_queue;
regs *registers;
uint64_t *rat; 
sched_q *schedule_queue;
rob_queue *r_queue;
proc_inst_t **scoreboard;
       
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
    proc_settings.k0 = k0; proc_settings.k1 = k1; proc_settings.k2 = k2; proc_settings.f = f; proc_settings.rob = rob; proc_settings.preg = preg;
    inflight_queue = malloc(sizeof(inflight_q));
    registers = malloc(sizeof(regs));
    rat = malloc(sizeof(uint64_t)*ARF); 
    schedule_queue = malloc(sizeof(sched_q));
    r_queue = malloc(sizeof(rob_queue));
    scoreboard = malloc(sizeof(proc_inst_t*)*(proc_settings.k0+proc_settings.k1+proc_settings.k2));
    
    inflight_queue->ifq = malloc(sizeof(proc_inst_t)*proc_settings.rob);
    inflight_queue->head = UINT_MAX; inflight_queue->tail = UINT_MAX;

    uint64_t num_regs = 8*proc_settings.f + ARF;
    uint64_t num_bits = num_regs%(sizeof(uint64_t)*CHAR_BIT)!=0 ? sizeof(uint64_t) : num_regs/CHAR_BIT; //will not work if not power of 2
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

    for(int i=0; i<ARF; i++)
        rat[i] = i;
   
    schedule_queue->num_entries = 0;
    schedule_queue->head = NULL; schedule_queue->tail = NULL;

    r_queue->rob = malloc(sizeof(rob_entry)*proc_settings.rob);
    num_bits = proc_settings.rob%(sizeof(uint64_t)*CHAR_BIT)!=0 ? sizeof(uint64_t) : proc_settings.rob/CHAR_BIT; //will not work if not power of 2
    r_queue->ready_bit = malloc(num_bits);
    memset(r_queue->ready_bit, 0, num_bits);
    r_queue->head = r_queue->tail = UINT_MAX;
    
    for(size_t i=0; i<(proc_settings.k0+proc_settings.k1+proc_settings.k2); i++)
        scoreboard[i] = NULL;
}

void dispatch(proc_inst_t *instance)
{
    sched_q_entry *temp = malloc(sizeof(sched_q_entry));
    rob_entry r;
    
    temp->instr = instance;
    instance->sqe = temp;
    r.instr = instance;
    instance->re = &r;

    temp->fu = instance->op_code;
    for(int i=0; i<2; i++)
        temp->src_preg[i] = (instance->src_reg[i]==INT_MAX) ? UINT_MAX : rat[instance->src_reg[i]];
    r.dest_areg = instance->dest_reg;
    r.prev_preg = (instance->dest_reg==INT_MAX) ? UINT_MAX : rat[instance->dest_reg];
    if(instance->dest_reg!=INT_MAX){
        rat[instance->dest_reg] = registers->head->number;
        free_pregs *t = registers->head;
        registers->head = registers->head->next;
        if(registers->head) registers->head->prev = NULL;
        free(t);
        registers->num_free_pregs--;
        clearbit(registers->ready_bit, rat[instance->dest_reg]);
    }
    r.curr_preg = (instance->dest_reg==INT_MAX) ? UINT_MAX : rat[instance->dest_reg];
    temp->dest_preg = (instance->dest_reg==INT_MAX) ? UINT_MAX : rat[instance->dest_reg];

    circular_enqueue(&r_queue->head, &r_queue->tail, proc_settings.rob); r_queue->rob[r_queue->tail] = r;
    
    clearbit(r_queue->ready_bit, r_queue->tail);

    temp->prev = NULL;
    temp->next = NULL;
    if(schedule_queue->head==NULL){
        schedule_queue->head = temp;
        schedule_queue->head->prev = NULL;
    }
    else{
        schedule_queue->tail->next = temp;
        temp->prev = schedule_queue->tail;
    }
    schedule_queue->tail = temp;
    schedule_queue->num_entries++;
}


void fire(uint64_t cycle, proc_stats_t *p_stats)
{
    int f0, f1;
    sched_q_entry *t = schedule_queue->head;
    while(t!=NULL){
        f0=0; f1=0;
        if(t->src_preg[0]==UINT_MAX) f0=1; if(t->src_preg[1]==UINT_MAX) f1=1;
        for(size_t i=r_queue->head;;){
            if(!f0 && t->src_preg[0]==r_queue->rob[i].curr_preg && testbit(r_queue->ready_bit, i)) f0=1;
            if(!f1 && t->src_preg[1]==r_queue->rob[i].curr_preg && testbit(r_queue->ready_bit, i)) f1=1;
            if(i==r_queue->tail) break;
            if(i==proc_settings.rob-1 && r_queue->tail<r_queue->head) i=0;
            else i++;
        }
        if( ( f0||testbit(registers->ready_bit, t->src_preg[0]) ) && ( f1||testbit(registers->ready_bit, t->src_preg[1]) ) ){
            int offset = (t->fu==2 ? (proc_settings.k0+proc_settings.k1) : (t->fu==1 ? proc_settings.k0 : 0));
            for(int i=offset; i<(t->fu==2 ? (proc_settings.k0+proc_settings.k1+proc_settings.k2) : (t->fu==1 ? (proc_settings.k0+proc_settings.k1) : proc_settings.k0)); i++)
                if(scoreboard[i]==NULL){
                    //printf("SCHED Inst Num: %"PRIu64"\n", t->instr->number);
                    p_stats->avg_inst_fired++;
                    scoreboard[i] = t->instr;
                    t->instr->scoreboarde = i;
                    t->instr->fire_cycle = cycle;
                    break;
                }
        }
        t = t->next;
    }
}

void complete(uint64_t cycle)
{
    sched_q_entry *sqe;
    for(int i=0; i<(proc_settings.k0+proc_settings.k1+proc_settings.k2); i++){
        if(scoreboard[i]!=NULL){
            proc_inst_t *instr = scoreboard[i];
            for(size_t i=r_queue->head; ;){
                if(instr==r_queue->rob[i].instr){
                    //printf("EX : Type: %d Inst_num: %"PRIu64"\n", instr->op_code, instr->number);
                    setbit(r_queue->ready_bit, i);
                    break;
                }
                if(i==r_queue->tail) break;
                if(i==proc_settings.rob-1 && r_queue->tail<r_queue->head) i=0;
                else i++;
            }
            instr->complete_cycle = cycle;
            sqe = scoreboard[i]->sqe;
            if(schedule_queue->num_entries>1){
                if(sqe==schedule_queue->head){
                    schedule_queue->head = schedule_queue->head->next;
                    schedule_queue->head->prev = NULL;
                }
                else if(sqe==schedule_queue->tail){
                    schedule_queue->tail = schedule_queue->tail->prev;
                    schedule_queue->tail->next = NULL;
                }
                else{
                    (sqe->prev)->next = sqe->next;
                    (sqe->next)->prev = sqe->prev;
                }
            }
            free(sqe);
            schedule_queue->num_entries--;
            if(schedule_queue->num_entries==0){
                schedule_queue->head = NULL;
                schedule_queue->tail = NULL;
            }
            scoreboard[i]=NULL;
        }
    }
}

void retire(uint64_t cycle, proc_stats_t *p_stats){
    int flag = 0;
    while(r_queue->head!=UINT_MAX && testbit(r_queue->ready_bit, r_queue->head)){
        //printf("RU Inst: %"PRIu64"\n", r_queue->rob[r_queue->head].instr->number);
        p_stats->avg_inst_retired++;
        r_queue->rob[r_queue->head].instr->retire_cycle = cycle;
        if(r_queue->rob[r_queue->head].curr_preg!=UINT_MAX) setbit(registers->ready_bit, r_queue->rob[r_queue->head].curr_preg);
        if(r_queue->rob[r_queue->head].prev_preg >= ARF && r_queue->rob[r_queue->head].prev_preg!=UINT_MAX){
            registers->num_free_pregs++;
            free_pregs *new_preg = malloc(sizeof(free_pregs));
            free_pregs *t = registers->head;
            new_preg->number = r_queue->rob[r_queue->head].prev_preg;
            if(t==NULL){
                registers->head = new_preg;
                registers->tail = new_preg;
            }
            else{
                flag = 0;
                while(t->next!=NULL){ //to be replaced with binary search
                    if(new_preg->number > t->number && new_preg->number < t->next->number){
                        new_preg->next = t->next;
                        t->next->prev = new_preg;
                        t->next = new_preg;
                        new_preg->prev = t;
                        flag = 1;
                        break;
                    }
                    t = t->next;
                }
                if(!flag && new_preg->number<registers->head->number){
                    new_preg->next = registers->head;
                    registers->head->prev = new_preg;
                    new_preg->prev = NULL;
                    registers->head = new_preg;
                }
                else if(!flag && new_preg->number>registers->tail->number){
                new_preg->prev = registers->tail;
                registers->tail->next = new_preg;
                new_preg->next = NULL;
                registers->tail = new_preg;
                }
            }
        }
        circular_dequeue(&r_queue->head, &r_queue->tail, proc_settings.rob); 
        circular_dequeue(&inflight_queue->head, &inflight_queue->tail, proc_settings.rob); 
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
    uint64_t cycle = 1, number = 1; 
    int flag=0;
    while((inflight_queue->head!=UINT_MAX && inflight_queue->tail!=UINT_MAX) || cycle==1){
        if(flag && inflight_queue->head==inflight_queue->tail) break;
        //printf("------------------------------------------  Cycle %"PRIu64" 		 ---------------------------------\n", cycle);
        retire(cycle, p_stats);
        complete(cycle);
        fire(cycle, p_stats);
        for(int i=0; i<proc_settings.f && !flag; i++){
            //printf("Number of free pregs : %d, %d\n", registers->num_free_pregs, schedule_queue->num_entries);
            if(schedule_queue->num_entries < 2*(proc_settings.k0 + proc_settings.k1 + proc_settings.k2) && !isFull(r_queue->head, r_queue->tail, proc_settings.rob) && registers->num_free_pregs > 0){
                circular_enqueue(&inflight_queue->head, &inflight_queue->tail, proc_settings.rob);
                if(read_instruction(&(inflight_queue->ifq[inflight_queue->tail]))) {
                    p_stats->retired_instruction++;
                    inflight_queue->ifq[inflight_queue->tail].number = number++;
                    inflight_queue->ifq[inflight_queue->tail].dispatch_cycle = cycle;
                    inflight_queue->ifq[inflight_queue->tail].fire_cycle = 0;
                    inflight_queue->ifq[inflight_queue->tail].complete_cycle = 0;
                    inflight_queue->ifq[inflight_queue->tail].retire_cycle = 0;
                    inflight_queue->ifq[inflight_queue->tail].scoreboarde = -1;
                    inflight_queue->ifq[inflight_queue->tail].sqe = NULL;
                    inflight_queue->ifq[inflight_queue->tail].re = NULL;
                    if(inflight_queue->ifq[inflight_queue->tail].op_code==-1)
                        inflight_queue->ifq[inflight_queue->tail].op_code = 1;
                    if(inflight_queue->ifq[inflight_queue->tail].dest_reg==-1)
                        inflight_queue->ifq[inflight_queue->tail].dest_reg = INT_MAX;
                    for(int i=0; i<2; i++)
                        if(inflight_queue->ifq[inflight_queue->tail].src_reg[i]==-1) inflight_queue->ifq[inflight_queue->tail].src_reg[i] = INT_MAX;
                    dispatch(&(inflight_queue->ifq[inflight_queue->tail]));
                    //printf("DU: Inst Num %"PRIu64"\n", inflight_queue->ifq[inflight_queue->tail].number);
                }
                else{
                    //printf("Couldn't read from file\n");
                    flag=1;
                }
            }
            else break;
        }
        //print_inflight_queue();
        //print_rat();
        //print_sched_queue(); 
        //print_rob_queue();
        //if(cycle==20) exit(0);
        cycle++;
        //printf("---------------------------------------------------------------------------------------------------\n\n");
    }
    cycle--;
    p_stats->avg_inst_fired/=cycle;
    p_stats->avg_inst_retired/=cycle;
    p_stats->cycle_count = cycle;
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
    free(scoreboard); scoreboard = NULL;
    free(registers->ready_bit); registers->ready_bit = NULL;
    free_pregs *t = registers->head;
    while(registers->head!=NULL){
        t = registers->head;
        registers->head = registers->head->next;
        free(t);
    }
    registers->head = NULL; registers->tail = NULL;
    free(registers); registers = NULL;
    free(rat); rat = NULL; 
    free(r_queue->rob); free(r_queue->ready_bit); 
    free(r_queue);
    free(schedule_queue);
    free(inflight_queue->ifq); free(inflight_queue);
}
