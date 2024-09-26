#ifndef _MODEL_H
#define _MODEL_H

#include <iostream>
#include <string>

#include "cacheline.h"
#include "rfentry.h"
#include "scheduler.h"
#include "shared_ADT.h"
#include "action.h"
#include "types.h"
#include "nodestack.h"
#include "condition_variable.h"


class Model {
public:
	using storeList = shared::list<ModelAction *>;

private:
	Scheduler *scheduler;
    std::atomic_int execution_num;

	//should be reset on rollback
	void* cxl_mapping;
	modelclock_t next_sequence_num;
    shared::vector<shared::string> placeholder_data;
	shared::hashmap<void *, storeList> obj_to_wr;
	shared::hashmap<uintptr_t, CacheLine> obj_to_cl;
	//stores the last model clock after which the process crashed
	shared::hashmap<process_id_t, modelclock_t> crashes;
    shared::hashmap<pthread_mutex_t*, Mutex*> mutex_map;
    shared::hashmap<pthread_cond_t*, ConditionVariable*> cond_map;
    NodeStack* nodestack;
    bool rollback_again;

	void process_store_buffer();

	void reset_execution_data();

	modelclock_t get_next_sequence_num() {return next_sequence_num++; }

	process_id_t get_process_id(ModelAction *action);
	
	CacheLine &get_cacheline(void *addr);

	storeList &get_storelist(void *addr);

	modelclock_t find_crashed_write(const shared::hashmap<process_id_t, modelclock_t> &curr_crashes, const storeList &stores, storeList::reverse_iterator itr, modelclock_t lb); 

	void read_crashed_update_cacheline(const storeList &stores, storeList::reverse_iterator itr, Range &r, rfEntry &e);
	
	bool has_unflushed_write(void *addr, process_id_t pid);

	bool should_crash();

    void record_crash_state(process_id_t);

public:
    Model(Scheduler *s, void* cxl): scheduler(s), execution_num(1), cxl_mapping(cxl), next_sequence_num(0), nodestack(new NodeStack), rollback_again(true) {}
    ~Model() { delete nodestack; }

    uint64_t action(ModelAction* action);
    
    void evict_store(ModelAction* action);
    
	void evict_clflush(ModelAction* action);
    
	void build_may_read_from(ModelAction *read, shared::vector<rfEntry> &rfset);

    void terminate_early();

    void finish_execution();

    Scheduler *get_scheduler() { return scheduler; }

    NodeStack* get_node_stack() { return nodestack; }

    // returns whether to rollback again
    bool wait_for_next_execution(int num);

    void print_execution_summary();

    shared::vector<shared::string> &get_placeholder_data() { return placeholder_data; };

    shared::hashmap<pthread_mutex_t*, Mutex*>* get_mutex_map() { return &mutex_map; }

    shared::hashmap<pthread_cond_t*, ConditionVariable*>* get_cond_map() { return &cond_map; }

	void *get_cxl_mapping() {
		return cxl_mapping;
	}

	bool is_crashed(process_id_t pid);
	
	void do_read(rfEntry &e);

    int decision_point(int numchoices) { return nodestack->explore_next(numchoices)->get_choice(); }

    void insert_crash();
};

extern Model *model;

inline void * alignAddress(void * addr) {
		uintptr_t address = (uintptr_t) addr;
			return (void *) (address & ~((uintptr_t)7));
}

#endif
