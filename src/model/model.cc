#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include <atomic>
#include <cstring>
#include <string>

#include "model.h"
#include "scheduler.h"
#include "executor.h"

Model *model;
mspace shared_space;
mspace snapshot_space;

void rfEntry::dump() {
	printf("cacheline: (%d, %d), ", cl.getBegin(), cl.getEnd()); 
	printf("crashes: [");
	for (auto pid: crashes)
		printf("%d, ", pid);
	printf("], ");

	printf("crashed cachlines:{");
	for (auto &pair: crashed_cls) {
		printf("%d: (%d, %d), ", pair.first, pair.second.getBegin(), pair.second.getEnd());
	}
	printf("}, ");

	printf("writes: [");
	for (uint i = 0; i < overlaps.size(); i++) {
		auto write = overlaps[i];
		if (!write)
			continue;
		printf("(+%u, val=%ld, seq=%u), ", i<<3, write->get_value(), write->get_seq_num());
	}
	printf("]\n");
}

uint64_t rfEntry::get_read_value(void *read_location) {
	uint64_t value = 0;
	for (int i= (int)overlaps.size()-1; i >= 0; i--) {
		value = value << 8;
		auto write = overlaps[i];
		if (!write)
			continue;
		int offset = i + (char *)read_location - (char *)write->get_location();
		uint64_t writevalue = write->get_value() >> (8 * offset);
		value |= writevalue & 0xff;
	}
	return value;
}

uint64_t Model::action(ModelAction* action) {
    scheduler->assert_active();
	if (is_crashed(process_id)) {
		scheduler->process_crash();
		exit(EXIT_SUCCESS);
	}

    Thread* curr_thread = scheduler->current_thread();
    curr_thread->set_pending(action);
    scheduler->yield();
    execute(action);
    curr_thread->set_pending(nullptr);

	uint64_t val = action->get_value();
    delete action; 

	process_store_buffer();
	return val;
}

bool Model::is_crashed(process_id_t pid) {
	return crashed_proc.find(pid) != crashed_proc.end();
}

void Model::process_store_buffer() {
	//placeholder store buffer policy, to be changed later
	uint num_to_pop = rand()%EVICT_MAX;
	uint thread_count = scheduler->get_thread_count();
	uint thread_to_pop = rand()%thread_count;
	for (uint i = 0; i < thread_count; i++) {
		if (!is_crashed(scheduler->get_thread(thread_to_pop)->get_process_id()))
			break;
		thread_to_pop = (thread_to_pop+1) % thread_count;
	}
	
	ThreadMemory *mem = scheduler->get_thread(thread_to_pop)->get_thread_memory();
	for (uint j = 0; j < num_to_pop; j++) {
		if (!mem->pop_from_store_buffer())
			break;
	}
}

process_id_t Model::get_process_id(ModelAction *action) { 
	return scheduler->get_thread(action->get_thread_id())->get_process_id(); 
}

void Model::do_read(rfEntry &e) {
	for (auto pid: e.crashes) {
		if (!is_crashed(pid)) {
			crash_count++;
			record_crash_state(pid);
		}
	}

	obj_to_cl[e.cl.getId()] = e.cl;
	for (auto itr: e.crashed_cls) {
		CacheLine &crashed_cl = itr.second;
		crashed_proc[itr.first][crashed_cl.getId()] = crashed_cl;
	}
}

CacheLine &Model::get_cacheline(void *addr)  { 
	uintptr_t id = getCacheID(addr);
	auto itr = obj_to_cl.try_emplace(id, id).first;
	return itr->second; 
}

CacheLine &Model::get_cacheline(void *addr, cachelineMap &cl_map)  { 
	uintptr_t id = getCacheID(addr);
	auto itr = cl_map.try_emplace(id, id).first;
	return itr->second; 
}

Model::storeList &Model::get_storelist(void *addr)  { 
	void *aligned = alignAddress(addr);
	auto itr = obj_to_wr.try_emplace(aligned).first;
	return itr->second; 
}

void Model::evict_store(ModelAction* action) {
    assert(action->get_type() == NONATOMIC_STORE);
	action->set_seq_num(get_next_sequence_num());
    get_storelist(action->get_location()).push_back(action);
}

bool Model::has_unflushed_write(void *addr, process_id_t pid) {
	storeList &stores = get_storelist(addr);
	modelclock_t cl_begin = get_cacheline(addr).getBegin();
	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		if (store->get_seq_num() <= cl_begin)
			break;
		if (get_process_id(store) == pid)
			return true;
	}

	return false;
}

void Model::evict_clflush(ModelAction* action) {
    assert(action->get_type() == CACHE_CLFLUSH || action->get_type() == CACHE_CLFLUSHOPT);
    insert_crash();
	modelclock_t seq_num = get_next_sequence_num();
	action->set_seq_num(seq_num);
	void *addr = action->get_location();
	get_cacheline(addr).setBegin(seq_num);
}

void Model::build_may_read_from(ModelAction *read, shared::vector<rfEntry> &rfset) {	
	CacheLine &cl = get_cacheline(read->get_location());
	
	uint numslotsleft = read->get_size();
	shared::vector<ModelAction *> overlaps(numslotsleft);
	if(scheduler->get_thread(read->get_thread_id())->get_thread_memory()->get_last_write(read, overlaps, numslotsleft)) {
		rfset.push_back(rfEntry(overlaps, cl, numslotsleft));
		return;
	}

	storeList &stores = get_storelist(read->get_location());
	shared::vector<rfEntry> seedWrites;
	seedWrites.push_back(rfEntry(overlaps, cl, numslotsleft));
	process_id_t rpid = get_process_id(read);
		
	for (auto itr = stores.rbegin(); itr != stores.rend(); itr++) {
		ModelAction *store = *itr;
		process_id_t wpid = get_process_id(store);
		if (seedWrites.empty())
			break;

		for (uint i=0; i<seedWrites.size(); i++) {
			auto &w = seedWrites[i];
			uint &seedSlotsleft = w.numslotsleft;
			auto &seedOverlaps = w.overlaps;
			auto &seedCrashes = w.crashes;
			auto citr = seedCrashes.begin();
			for (; citr != seedCrashes.end() && *citr != wpid; citr++) {}
			auto citr2 = crashed_proc.find(wpid);

			//running processes
			if (citr == seedCrashes.end() &&
				citr2 == crashed_proc.end()) {
				rfEntry copy{w};
				if (get_overlaps(seedOverlaps, store, read, seedSlotsleft)) {
					if (wpid != rpid) {
						modelclock_t seq_num = get_next_sequence_num();
						w.cl.setBegin(seq_num);
				
						//consider crashing the writing process
						if (!scheduler->get_thread(store->get_thread_id())->is_completed() && copy.crashes.size() + crash_count < MAX_CRASHES_PER_EXECUTION) {
							copy.crashes.push_back(wpid);
							seedWrites.push_back(copy);
						}
					}
				}
			} else { //crashed processes
				if (w.crashed_cls.find(wpid) == w.crashed_cls.end()) 
					w.crashed_cls[wpid] = citr != seedCrashes.end() ? cl : get_cacheline(read->get_location(), citr2->second);
				CacheLine &seedCL = w.crashed_cls[wpid];

				if (store->get_seq_num() <= seedCL.getBegin()) { //must have persisted
					if (get_overlaps(seedOverlaps, store, read, seedSlotsleft))
						do_crashed_read(read, store, seedCL);
				} else if (seedCL.getEnd() == 0 || store->get_seq_num() <= seedCL.getEnd()) { //may have persisted
					rfEntry copy(w);
					if (get_overlaps(copy.overlaps, store, read, copy.numslotsleft)) {
						do_crashed_read(read, store, copy.cl);
						if (copy.numslotsleft == 0)
							rfset.push_back(copy);
						else
							seedWrites.push_back(copy);
					}
				}
			}
		}

		//move full seedWrites to rfset
		for (uint i = 0; i < seedWrites.size(); i++) {
			auto &w = seedWrites[i];
			if (w.numslotsleft == 0) {
				rfset.push_back(w);
				seedWrites[i] = seedWrites.back();
				seedWrites.pop_back();
			}
		}
	}
	for (auto w: seedWrites)
		rfset.push_back(w);
}

void Model::do_crashed_read(ModelAction *read, ModelAction *write, CacheLine &cl) {
	modelclock_t write_seq = write->get_seq_num();
	if (cl.getBegin() < write_seq)
		cl.setBegin(write_seq);
	storeList &stores = get_storelist(read->get_location());
	
	storeList::const_iterator itr = stores.begin(); 
	for (; itr != stores.end(); itr++) 
		if (*itr == write)
			break;
	assert(itr != stores.end());

	itr++;
	uintptr_t wbot = (uintptr_t) write->get_location();
	uintptr_t wtop = wbot + write->get_size();

	//find next overlapping write
	for (; itr != stores.end(); itr++) {
		uintptr_t wbot2 = (uintptr_t) (*itr)->get_location();
		uintptr_t wtop2 = wbot2 + (*itr)->get_size();
		if (wtop > wbot2 && wbot < wtop2)
			break;
	}

	if (itr != stores.end()) {
		modelclock_t next_write_seq = (*itr)->get_seq_num();
		if (cl.getEnd() == 0 || cl.getEnd() >= next_write_seq)
			cl.setEnd(next_write_seq);
	}
}

void Model::print_execution_summary() {
		printf("\nExecution Summary\n");
        printf("stores: \n");
        for (auto &itr: obj_to_wr) {
			printf("aligned loc %p [", itr.first);
			for (auto s: itr.second) {
				int offset = (char *) s->get_location() - (char *) itr.first;
				printf("(+%d: val=%ld, seq=%u), ", offset, s->get_value(), s->get_seq_num());
			}
			printf("]\n");
		}
        printf("\n");

        printf("cachelines: \n");
		for (auto &pair: obj_to_cl)
			printf("%p: (%d, %d), ", (void *)pair.first, pair.second.getBegin(), pair.second.getEnd()); 
        printf("\n\n");
		
		for (auto &cpair: crashed_proc) {
			printf("cachelines for crashed process %u: \n", cpair.first);
			for (auto &pair: cpair.second)
				printf("%p: (%d, %d), ", (void *)pair.first, pair.second.getBegin(), pair.second.getEnd()); 
        	printf("\n\n");
		}
        
		printf("placeholder data: \n");
        for (auto &s: placeholder_data)
            printf("%s, ", s.c_str());
        printf("\n");
}

void Model::terminate_early() {
    rollback_again = false;
    finish_execution();
    _Exit(EXIT_FAILURE);
}

void Model::finish_execution() {
    bool isLast = !scheduler->finalize();
	exit_count.fetch_add(1);
    printf("process %d done, is last %d\n", process_id, isLast);	
    int num = execution_num.load();

    if (isLast) {
		print_execution_summary();
        rollback_again = rollback_again && num+1 <= MAX_EXECUTION && nodestack->has_another_execution();
        if (rollback_again) {
            printf("-------------------------- execution %d--------------------------\n", num+1);
			reset_execution_data();
            nodestack->reset_execution();
        }

        crash_count = 0;
        exit_count = 0;
        scheduler->reset();
        execution_num.store(num+1);
    }
}

bool Model::wait_for_next_execution(int num) {
    if (num > MAX_EXECUTION || !rollback_again) {
        return false;
    }

    while (execution_num.load() < num) {
        sleep(0);
    }

    return rollback_again;
}

void Model::reset_execution_data() {
		memset(cxl_mapping, 0, CXL_MEM_SIZE);
		next_sequence_num = 0;
		for (auto itr: obj_to_wr)
			for (auto s: itr.second)
				delete s;
        obj_to_wr.clear();
        placeholder_data.clear();
		obj_to_cl.clear();
		crashed_proc.clear();
}

void Model::record_crash_state(process_id_t pid) {
	cachelineMap &map = crashed_proc[pid];
	for (auto &pair: obj_to_cl)
		if (has_unflushed_write((void*)pair.first, process_id))
			map[pair.first] = pair.second;

}

bool Model::should_crash() {
    if (crash_count < MAX_CRASHES_PER_EXECUTION && decision_point(2) == 0) {
        crash_count++;
        return true;
    }
    return false;
}

void Model::insert_crash() {
    if (!should_crash()) {
        return;
    }
	
	scheduler->process_crash();
	record_crash_state(process_id);
	exit(EXIT_SUCCESS);
}
