#include <iostream>
#include <string>

#include "model.h"
#include "executor.h"
#include "threads.h"

Thread* get_thread(ModelAction* action) {
    return model->get_scheduler()->get_thread(action->get_thread_id());
}

void thread_wait(Thread* thread) {
    assert(thread->get_pending());
    assert(thread->get_pending()->get_type() == PTHREAD_JOIN ||
        thread->get_pending()->get_type() == ATOMIC_LOCK ||
        thread->get_pending()->get_type() == ATOMIC_WAIT);

    printf("thread %d waiting\n", thread->get_thread_id());
    thread->set_state(THREAD_BLOCKED);
    model->get_scheduler()->yield();
}

void execute(ModelAction* action) {
    switch(action->get_type()) {
    case THREAD_START:
    case THREAD_YIELD:
        break;
    case THREAD_FINISH: {
        Thread* curr_thread = get_thread(action); 
        model->get_scheduler()->wake_all_threads_waiting_on(get_thread(action));
        curr_thread->finalize();
        break;
    }
    case THREADONLY_FINISH: { 
        Thread* curr_thread = get_thread(action);  
        curr_thread->ret_val = action->get_location();
        model->get_scheduler()->wake_all_threads_waiting_on(get_thread(action));
        curr_thread->finalize();
        break;
    }
    case PLACEHOLDER: {
        char* s = (char*)action->get_location();
        std::cout << "process " << model->get_scheduler()->get_process_id() << ", " << "thread "
            << model->get_scheduler()->get_thread_id() << ", " << s << std::endl;
        model->get_placeholder_data().push_back(s);
        break;
    }
    case PTHREAD_CREATE: {
        struct pthread_params* params = (struct pthread_params*)action->get_value();
		thread_id_t tid = model->get_scheduler()->new_thread(params->func, params->arg);
		get_thread(action)->get_thread_memory()->add_to_store_buffer(new ModelAction(NONATOMIC_STORE, action->get_location(), tid, memory_order_relaxed, sizeof(thread_id_t), "pthread_create"));
        *(thread_id_t*)action->get_location() = tid; 
        break;
    }
    case PTHREAD_JOIN: {
        Thread* curr_thread = get_thread(action);
        Thread* thread = action->get_thread();

        assert(thread->get_process_id() == process_id);

        printf("thread %d joining %d\n", thread_id, thread->get_thread_id());
        if (thread->get_state() != THREAD_COMPLETED) {
            thread_wait(curr_thread);
        }
        printf("%d joined %d completed\n", thread_id, thread->get_thread_id());
        break;
    }
    case ATOMIC_TRYLOCK: {
        Thread* curr_thread = get_thread(action);
        Mutex* mutex = action->get_mutex();
        Thread* owner = mutex->get_owner();

        if (!owner) {
            mutex->set_owner(curr_thread);
            action->set_value(true);
            break;
        }

        action->set_value(false);
        break;
    }
    case ATOMIC_LOCK: {
        Thread* curr_thread = get_thread(action);
        Mutex* mutex = action->get_mutex();
        Thread* owner = mutex->get_owner();

        if (!owner) {
            mutex->set_owner(curr_thread);
            break;
        }

        if (curr_thread == owner) {
            if (mutex->get_mutex_type() == PTHREAD_MUTEX_RECURSIVE) {
                mutex->increment_lock_count();
            } else if (mutex->get_mutex_type() == PTHREAD_MUTEX_ERRORCHECK) {
                errno = EDEADLK;
            } else {
                printf("DEADLOCK\n");
                abort();
            }
            break;
        }

        while (mutex->get_owner())
            thread_wait(curr_thread);

        assert(!mutex->get_owner());
        mutex->set_owner(curr_thread);
        break;
    }
    case ATOMIC_WAIT:
    case ATOMIC_TIMEDWAIT:
    case ATOMIC_UNLOCK: {
        Thread* curr_thread = get_thread(action);
        Mutex* mutex = action->get_mutex();
        Thread* owner = mutex->get_owner();

        if (curr_thread != owner) {
            errno = EPERM;
            break;
        }

        if (mutex->get_mutex_type() == PTHREAD_MUTEX_RECURSIVE && mutex->get_recursive_lock_count() > 0) {
            mutex->decrement_lock_count();
            break;
        }

        mutex->set_owner(nullptr);
        model->get_scheduler()->wake_all_threads_waiting_on(mutex);

        if (action->get_type() == ATOMIC_WAIT) {
            thread_wait(curr_thread);
        }
        break;
    }
    case ATOMIC_NOTIFY_ONE: {
        ConditionVariable* cv = action->get_cond();
        model->get_scheduler()->wake_thread_waiting_on(cv);
        break;
    }
    case ATOMIC_NOTIFY_ALL: {
        ConditionVariable* cv = action->get_cond();
        model->get_scheduler()->wake_all_threads_waiting_on(cv);
        break;
    }
    case ATOMIC_INIT:
	case ATOMIC_STORE: 
    case NONATOMIC_STORE:
    case ATOMIC_RMW: {
		ModelAction *storeAction = new ModelAction(*action); //old copy will be deleted
		get_thread(storeAction)->get_thread_memory()->add_to_store_buffer(storeAction);
        if (action->get_type() == ATOMIC_RMW || action->is_seq_cst()) {
            ThreadMemory* memory = get_thread(action)->get_thread_memory();
            memory->empty_store_buffer();
            memory->empty_flush_buffer();
        }
		break;
    }
    case ATOMIC_RMWR: {
		ThreadMemory* memory = get_thread(action)->get_thread_memory();
        memory->empty_store_buffer();
        memory->empty_flush_buffer();
        [[fallthrough]];
    }
	case ATOMIC_LOAD:
	case NONATOMIC_LOAD: {
		shared::vector<rfEntry> rfset;
		model->build_may_read_from(action, rfset);

		printf("thread %u rfset %p: {\n", action->get_thread_id(), action->get_location());
		for (auto &entry: rfset) {
			printf("\t");
			entry.dump();
		}
		printf("}\n");

		assert(rfset.size() != 0);
		int index = model->decision_point(rfset.size());
		auto chosen = rfset[index];
		uint64_t read_value = chosen.get_read_value(action->get_location());
		printf("choose option %d of rfset, val=%lx\n", index, read_value);
		model->do_read(chosen);

		action->set_value(read_value);
		break;
	}
    case CACHE_SFENCE:
    case CACHE_CLFLUSH:
    case CACHE_CLFLUSHOPT: {
		ModelAction *flushAction = new ModelAction(*action); //old copy will be deleted
		get_thread(flushAction)->get_thread_memory()->add_to_store_buffer(flushAction); 
		break;
	}
	case CACHE_MFENCE: {
		ThreadMemory* memory = get_thread(action)->get_thread_memory();
        memory->empty_store_buffer();
        memory->empty_flush_buffer(); 
		break;
	}
    case ATOMIC_CAS_FAILED: {
        break;
    }
	default:
		assert(false && "not implemented");
    }
}
