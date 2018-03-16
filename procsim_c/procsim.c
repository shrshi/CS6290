#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "procsim.h"

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
    
    uint64_t num_regs = 8*proc_settings.f;
    uint64_t num_bits = num_regs%(sizeof(uint64_t)*CHAR_BIT)!=0 ? sizeof(uint64_t) : num_regs/CHAR_BIT; //will not work if not power of 2
    regs *registers = malloc(sizeof(regs));
    registers->ready_bit = malloc(num_bits);
    registers->free_bit = malloc(num_bits);
    registers->free_index = ARF+1;
    memset(registers->ready_bit, 0, num_bits);
    memset(registers->free_bit, 1, num_bits);
    for(int i=0; i<ARF; i++){
        setbit(registers->ready_bit, i);
        clearbit(registers->free_bit, i);
    }
    
    uint64_t *rat = malloc(sizeof(uint64_t)*ARF); 
    for(int i=0; i<ARF; i++)
        rat[i] = i;
   
    sched_q *schedule_queue = malloc(sizeof(sched_q));
    schedule_queue->table = malloc(sizeof(sched_q_entry)*proc_settings.rob);  
    schedule_queue->ptr = schedule_queue->table;

    rob_queue *r_queue = malloc(sizeof(rob_queue));
    r_queue->rob = malloc(sizeof(rob_entry)*proc_settings.rob);
    num_bits = proc_settings.rob%(sizeof(uint64_t)*CHAR_BIT)!=0 ? sizeof(uint64_t) : proc_settings.rob/CHAR_BIT; //will not work if not power of 2
    r_queue->ready_bit = malloc(num_bits);
    memset(r_queue->ready_bit, 0, num_bits);
    r_queue->exception_bit = malloc(num_bits);
    memset(r_queue->exception_bit, 0, num_bits);
    r_queue->head = r_queue->tail = r_queue->rob;
    
    uint64_t *scoreboard = malloc(sizeof(uint64_t));
    memset(scoreboard, 0, sizeof(uint64_t));
}

void dispatch(proc_inst_t instance)
{
    schedule_queue->table[schedule_queue->ptr].fu = instance.op_code;
    for(int i=0; i<2; i++)
        schedule_queue->table[schedule_queue->ptr].src_preg[i] = rat[instance.src_reg[i]];
    r_queue->rob[r_queue->tail].dest_areg = instance.dest_reg;
    r_queue->rob[r_queue->tail].prev_preg = rat[instance.dest_reg];
    rat[instance.dest_reg] = free_index;
    schedule_queue->table[schedule_queue->ptr].dest_preg = free_index;
    clearbit(registers->ready_bit, free_index);
    clearbit(registers->free_bit, free_index);
    clearbit(r_queue->ready_bit, 


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
    proc_inst_t instance;
    if(read_instruction(&instance)){
        dispatch(instance);
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
