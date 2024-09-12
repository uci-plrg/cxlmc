#include <iostream>
#include <string>

#include "model.h"
#include "executor.h"
#include "threads.h"

Thread* get_thread(ModelAction* action) {
    return model->get_scheduler()->get_thread(action->get_thread_id());
}

void execute(ModelAction* action) {
    switch(action->get_type()) {
    case THREAD_START:
    case THREAD_YIELD:
        break;
    case THREAD_FINISH: {
        Thread* curr_thread = get_thread(action); 
        model->get_scheduler()->wake_threads_waiting_on(get_thread(action));
        curr_thread->finalize();
        break;
    }
    case THREADONLY_FINISH: { 
        Thread* curr_thread = get_thread(action);  
        curr_thread->ret_val = action->get_location();
        model->get_scheduler()->wake_threads_waiting_on(get_thread(action));
        curr_thread->finalize();
        break;
    }
    case PLACEHOLDER: {
        std::string* s = (std::string*)action->get_location();
        std::cout << "process " << model->get_scheduler()->get_process_id() << ", " << "thread "
            << model->get_scheduler()->get_thread_id() << ", " << *s << std::endl;
        model->get_placeholder_data().push_back(shared::string(s->c_str()));
        break;
    }
    case PTHREAD_CREATE: {
        struct pthread_params* params = (struct pthread_params*)action->get_value();
        *(int*)action->get_location() = model->get_scheduler()->new_thread(params->func, params->arg);
        break;
    }
    case PTHREAD_JOIN: {
        Thread* curr_thread = get_thread(action);
        Thread* thread = (Thread*)action->get_location();

        assert(thread->get_process_id() == process_id);

        printf("thread %d joining %d\n", thread_id, thread->get_thread_id());
        if (thread->get_state() != THREAD_COMPLETED) {
            curr_thread->set_state(THREAD_BLOCKED);
            model->get_scheduler()->yield();
        }
        printf("%d joined %d completed\n", thread_id, thread->get_thread_id());
        break;
    }
    case NONATOMIC_STORE: {
		ModelAction *storeAction = new ModelAction(*action); //old copy will be deleted
		get_thread(storeAction)->get_thread_memory()->add_to_store_buffer(storeAction); 
		break;
    }
	case NONATOMIC_LOAD: {
		shared::vector<ModelAction *> rfset;
		model->build_may_read_from(action, rfset);
		uint64_t loaded = VALUE_NONE;
		if (rfset.size() != 0) {
			int index = model->get_node_stack()->explore_next(rfset.size())->get_choice();
			loaded = rfset[index]->get_value();
		}

		model->do_read(action, get_thread(action)->get_process_id(), loaded);
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
	default:
		assert(false && "not implemented");
    }
}
