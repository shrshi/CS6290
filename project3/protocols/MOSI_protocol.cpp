#include "MOSI_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOSI_protocol::MOSI_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    this->state = MOSI_CACHE_I;
}

MOSI_protocol::~MOSI_protocol ()
{    
}

void MOSI_protocol::dump (void)
{
    const char *block_states[10] = {"X","I","S","O","M","IS","IM","SM","OM","OML"};
    fprintf (stderr, "MOSI_protocol - state: %s\n", block_states[state]);
}

void MOSI_protocol::process_cache_request (Mreq *request)
{
	switch (state) {
        case MOSI_CACHE_I: do_cache_I(request); break;
        case MOSI_CACHE_S: do_cache_S(request); break;
        case MOSI_CACHE_O: do_cache_O(request); break;
        case MOSI_CACHE_M: do_cache_M(request); break;
        case MOSI_CACHE_IS: do_cache_IS(request); break;
        case MOSI_CACHE_IM: do_cache_IM(request); break;
        case MOSI_CACHE_SM: do_cache_SM(request); break;
        case MOSI_CACHE_OM: do_cache_OM(request); break;
        case MOSI_CACHE_OML: do_cache_OML(request); break;
        default:
            fatal_error ("Invalid Cache State for MOSI Protocol\n");
    }
}

void MOSI_protocol::process_snoop_request (Mreq *request)
{
	switch (state) {
        case MOSI_CACHE_I: do_snoop_I(request); break;
        case MOSI_CACHE_S: do_snoop_S(request); break;
        case MOSI_CACHE_O: do_snoop_O(request); break;
        case MOSI_CACHE_M: do_snoop_M(request); break;
        case MOSI_CACHE_IS: do_snoop_IS(request); break;
        case MOSI_CACHE_IM: do_snoop_IM(request); break;
        case MOSI_CACHE_SM: do_snoop_SM(request); break;
        case MOSI_CACHE_OM: do_snoop_OM(request); break;
        case MOSI_CACHE_OML: do_snoop_OML(request); break;
        default:
            fatal_error ("Invalid Cache State for MOSI Protocol\n");
    }
}

inline void MOSI_protocol::do_cache_I (Mreq *request)
{
    switch(request->msg){
        case LOAD:
            send_GETS(request->addr);
            state = MOSI_CACHE_IS;
            Sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOSI_CACHE_IM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_IS (Mreq *request)
{
    switch(request->msg){
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_IM (Mreq *request)
{
    switch(request->msg){
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_S (Mreq *request)
{
    switch(request->msg){
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOSI_CACHE_SM;
            Sim->cache_misses++;
            break;
        default:    
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_SM (Mreq *request)
{
    switch(request->msg){
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_O (Mreq *request)
{
    switch(request->msg){
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOSI_CACHE_OM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message\n");
    }

}

inline void MOSI_protocol::do_cache_OM (Mreq *request)
{
    switch(request->msg){
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: O state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_OML (Mreq *request)
{
    switch(request->msg){
        case LOAD:
        case STORE:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_cache_M (Mreq *request)
{
    switch(request->msg){
        case LOAD:
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_I (Mreq *request)
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

inline void MOSI_protocol::do_snoop_IS (Mreq *request)
{
    switch(request->msg){
        case GETS:
        case GETM: break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOSI_CACHE_S;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IS state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_IM (Mreq *request)
{
    switch(request->msg){
        case GETS:
        case GETM: break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOSI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: IM state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_S (Mreq *request)
{
    switch(request->msg){
        case GETS: break;
        case GETM: state = MOSI_CACHE_I; break;
        case DATA: 
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_SM (Mreq *request)
{
    switch(request->msg){
        case GETS: 
        case GETM: break;
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOSI_CACHE_M; //O or M or OM would have sent this
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_M (Mreq *request)
{
    switch(request->msg){
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid);
            state = MOSI_CACHE_O;    
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            state = MOSI_CACHE_I;
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_O (Mreq *request)
{
    switch(request->msg){
        case GETS:
            send_DATA_on_bus(request->addr, request->src_mid);
            break;
        case GETM:
            send_DATA_on_bus(request->addr, request->src_mid);
            state = MOSI_CACHE_I;
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: O state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_OM (Mreq *request)
{
    switch(request->msg){
        case GETS: 
            send_DATA_on_bus(request->addr, request->src_mid);
            break;
        case GETM: 
            send_DATA_on_bus(request->addr, request->src_mid);
            state = MOSI_CACHE_OML;
            break;
        case DATA:
            fatal_error ("Should not see data for this line!  I have the line!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: OM state shouldn't see this message\n");
    }
}

inline void MOSI_protocol::do_snoop_OML (Mreq *request)
{
    switch(request->msg){
        case GETS:
        case GETM: break;
        case DATA:
            /** 
            * DATA could either be from OM (essentially O->M when others are in S)
            * or from SM (when OM exists with SM)
            */
            send_DATA_to_proc(request->addr);
            state = MOSI_CACHE_M;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: OML state shouldn't see this message\n");
    }
}
