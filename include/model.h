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
#include "condition_variable.h"

using ExtPtr = shared::Pair<void *, process_id_t>;
class Model {
public:
    using storeList = shared::list<ModelAction *>;

private:
    //persist across executions
    Scheduler *scheduler;
    std::atomic_int execution_num;
    unsigned crash_points;

    //should be reset on rollback
    void* cxl_mapping;
    modelclock_t next_sequence_num;
    shared::hashmap<void *, storeList> obj_to_wr;
    CacheLineStore obj_to_cl;
    //stores the last model clock after which the process crashed
    shared::hashmap<process_id_t, modelclock_t> crashes;
    shared::hashset<process_id_t> completed_procs;
    shared::hashmap<ExtPtr, Mutex*, ExtPtr::hash> mutex_map;
    shared::hashmap<ExtPtr, ConditionVariable*, ExtPtr::hash> cond_map;
    NodeStack* nodestack;
    bool rollback_again;

    int execution_num_save;
    char *ns_save;

    void process_store_buffer();

    void reset_execution_data();

    modelclock_t get_next_sequence_num() {return next_sequence_num++; }

    process_id_t get_process_id(ModelAction *action);

    storeList &get_storelist(void *addr);

    bool has_unflushed_write(void *addr, process_id_t pid);

    bool should_crash();

    void record_crash_state(process_id_t);

    void ensureInitialValue(ModelAction *action);

    void check_memory_poisoning(ModelAction *read);

public:
    Model(Scheduler *s): scheduler(s), execution_num(1), crash_points(0), cxl_mapping(NULL), next_sequence_num(0), nodestack(new NodeStack), rollback_again(true),
        execution_num_save(0), ns_save(nullptr) {
        obj_to_cl.init(s->get_process_count());
    }
    ~Model() { delete nodestack; }

    uint64_t action(ModelAction* action, bool yield=true);

    void evict_store(ModelAction* action);

    void evict_clflush(ModelAction* action);

    uint64_t build_read_from(ModelAction *read);

    void terminate_early();

    void finish_execution();

    void save_execution(int exec_num, char *filepath) {
        execution_num_save = exec_num;
        ns_save = filepath;
    }

    void load_nodestack(char *filepath);

    Scheduler *get_scheduler() { return scheduler; }

    NodeStack* get_node_stack() { return nodestack; }

    // returns whether to rollback again
    bool wait_for_next_execution(int num);

    void print_execution_summary();

    shared::hashmap<ExtPtr, Mutex*, ExtPtr::hash>* get_mutex_map() { return &mutex_map; }

    shared::hashmap<ExtPtr, ConditionVariable*, ExtPtr::hash>* get_cond_map() { return &cond_map; }

    void *get_cxl_mapping() { return cxl_mapping; }

    void set_cxl_mapping(void* mapping) { cxl_mapping = mapping; }

    bool mem_is_cxl(const void *);

    bool is_crashed(process_id_t pid) { return crashes.find(pid) !=crashes.end(); }

    bool is_completed(process_id_t pid) { return completed_procs.find(pid) !=completed_procs.end(); }

    inline bool is_live(process_id_t pid) { return !is_crashed(pid) && !is_completed(pid); }

    int decision_point(int numchoices, bool *at_backtrack=NULL);

    void insert_crash(process_id_t pid=process_id);

    void complete_process() { completed_procs.insert(process_id); }

    process_id_t get_crashed_process_count() { return crashes.size(); }

    process_id_t get_completed_process_count() { return completed_procs.size(); }

    int get_execution_num() { return execution_num.load(); }
};

extern Model *model;

inline void * alignAddress(void * addr) {
        uintptr_t address = (uintptr_t) addr;
            return (void *) (address & ~((uintptr_t)7));
}

#endif
