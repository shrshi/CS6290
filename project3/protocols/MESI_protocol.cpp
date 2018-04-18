#include "MESI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MESI_protocol::MESI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    this->state = MESI_CACHE_I;
}

MESI_protocol::~MESI_protocol ()
{    
}

void MESI_protocol::dump (void)
{
    const char *block_states[5] = {"X","I","S","E","M"};
    fprintf (stderr, "MESI_protocol - state: %s\n", block_states[state]);
}

void MESI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
        case MESI_CACHE_I: do_cache_I (request); break;
        case MESI_CACHE_S: do_cache_S (request); break;
        case MESI_CACHE_E: do_cache_E (request); break;
        case MESI_CACHE_M: do_cache_M (request); break;
        case MESI_CACHE_IS:
            switch (request->msg) {
                case LOAD:
                case STORE:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error("Should only have one outstanding request per processor!"); break;
                default:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error ("Client: I state shouldn't see this message\n");
                }
            break;
        case MESI_CACHE_IM:
            switch (request->msg) {
                case LOAD:
                case STORE:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error("Should only have one outstanding request per processor!"); break;
                default:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error ("Client: I state shouldn't see this message\n");
                }
            break;
        case MESI_CACHE_SM:
            switch (request->msg) {
                case LOAD:
                case STORE:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error("Should only have one outstanding request per processor!"); break;
                default:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error ("Client: S state shouldn't see this message\n");
                }
            break;
        default:
            fatal_error ("Invalid Cache State for MESI Protocol\n");
    }
}

void MESI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
        case MESI_CACHE_I: do_snoop_I (request); break;
        case MESI_CACHE_S: do_snoop_S (request); break;
        case MESI_CACHE_E: do_snoop_E (request); break;
        case MESI_CACHE_M: do_snoop_M (request); break;
        case MESI_CACHE_IS:
            switch(request->msg){
                case GETM:
                case GETS: break;
                case DATA:
                    send_DATA_to_proc(request->addr);
                    state = MESI_CACHE_E;
                    if(get_shared_line()) {
                        state = MESI_CACHE_S;
                        Sim->cache_to_cache_transfers++;
                    }
                    break; 
                default:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error ("Client: I state shouldn't see this message\n");
            }
            break;
        case MESI_CACHE_IM:
            switch(request->msg){
                case GETS:
                case GETM: break;
                case DATA:
                    send_DATA_to_proc(request->addr);
                    state = MESI_CACHE_M;
                    if(get_shared_line()) Sim->cache_to_cache_transfers++;
                    break; 
                default:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error ("Client: I state shouldn't see this message\n");
            }
            break;
        case MESI_CACHE_SM:
            switch(request->msg){
                case GETS:
                case GETM: break;
                case DATA:
                    send_DATA_to_proc(request->addr);
                    state = MESI_CACHE_M;
                    break; 
                default:
                    request->print_msg (my_table->moduleID, "ERROR");
                    fatal_error ("Client: I state shouldn't see this message\n");
            }
            break;
        default:
            fatal_error ("Invalid Cache State for MESI Protocol\n");
    }
}

inline void MESI_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_GETS(request->addr);
            state = MESI_CACHE_IS;
            Sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->addr);
            state = MESI_CACHE_IM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_S (Mreq *request)
{
    switch(request->msg){
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);
            state = MESI_CACHE_SM;
            break;  
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_E (Mreq *request)
{
    switch(request->msg){
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            state = MESI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_cache_M (Mreq *request)
{
    switch(request->msg){
        case LOAD:
        case STORE:
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_I (Mreq *request)
{
    switch(request->msg){
        case GETS:
        case GETM:
        case DATA: break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_S (Mreq *request)
{
    switch(request->msg){
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);
            break;
        case GETM:
            //set_shared_line();
            //send_DATA_on_bus(request->addr, request->src_mid);
            state = MESI_CACHE_I;
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_E (Mreq *request)
{
    switch(request->msg){
        case GETS:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MESI_CACHE_S;
            break;
        case GETM:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MESI_CACHE_I;
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MESI_protocol::do_snoop_M (Mreq *request)
{
    switch(request->msg){
        case GETS:
        case GETM:
            set_shared_line();
            send_DATA_on_bus(request->addr,request->src_mid);
            state = MESI_CACHE_I;
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}
