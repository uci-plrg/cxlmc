#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <string>

#include "cacheline.h"
#include "scheduler.h"
#include "shared_ADT.h"
#include "action.h"
#include "types.h"
#include "nodestack.h"

class Model {
	using storelist = shared::list<ModelAction *>;

    Scheduler *scheduler;
    std::atomic_int execution_num;

	//should be reset on rollback
	void* cxl_mapping;
	modelclock_t next_sequence_num;
    shared::vector<shared::string> placeholder_data;
	shared::hashmap<void *, storelist> obj_to_wr;
	shared::hashmap<uintptr_t, CacheLine> obj_to_cacheline;
    NodeStack* nodestack;

    int crash_count;
    bool rollback_again;

	void reset_execution_data();

	modelclock_t get_next_sequence_num() {return next_sequence_num++; }

	CacheLine &get_cacheline(void *addr);
	
	storelist &get_storelist(void *addr);

    void execute_crash();
public:
    Model(Scheduler *s, void* cxl): scheduler(s), execution_num(1), cxl_mapping(cxl), next_sequence_num(0), nodestack(new NodeStack),
        crash_count(0), rollback_again(true) {}
    ~Model() { delete nodestack; }

    uint64_t action(ModelAction* action);
    
    void evict_store(ModelAction* action);
    
	void evict_clflush(ModelAction* action);
    
    void finish_execution();

    Scheduler *get_scheduler() { return scheduler; }

    NodeStack* get_node_stack() { return nodestack; }

    // returns whether to rollback again
    bool wait_for_next_execution(int num);

    void print_execution_summary();

    shared::vector<shared::string> &get_placeholder_data() { return placeholder_data; };

	void *get_cxl_mapping() {
		return cxl_mapping;
	}

    int decision_point(int numchoices) { return nodestack->explore_next(numchoices)->get_choice(); }

    bool should_crash();

    void insert_crash();
};

extern Model *model;

inline void * alignAddress(void * addr) {
		uintptr_t address = (uintptr_t) addr;
			return (void *) (address & ~((uintptr_t)7));
}

#endif
