#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <string>

#include "cacheline.h"
#include "scheduler.h"
#include "shared_ADT.h"
#include "action.h"
#include "types.h"

class Model {
    Scheduler *scheduler;
    std::atomic_int execution_num;
    bool rollback;

	//should be reset on rollback
	void* cxl_mapping;
	modelclock_t next_sequence_num;
    shared::vector<shared::string> placeholder_data;
    shared::list<ModelAction *> store_list;
	shared::hashmap<uintptr_t, CacheLine> obj_to_cacheline;

	void reset_execution_data();

	modelclock_t get_next_sequence_num() {return next_sequence_num++; }

	CacheLine &get_cacheline(void *addr);

public:
    Model(Scheduler *s, void* cxl): scheduler(s), execution_num(1), rollback(true), cxl_mapping(cxl), next_sequence_num(0){}    

    void action(ModelAction* action);
    
    void add_to_store_list(ModelAction* action);
    
    void finish_execution();

    Scheduler *get_scheduler() { return scheduler; }

    bool should_rollback() { return rollback; }

    void print_execution_summary();

    shared::vector<shared::string> &get_placeholder_data() { return placeholder_data; };

	void *get_cxl_mapping() {
		return cxl_mapping;
	}
};

extern Model *model;

#endif
