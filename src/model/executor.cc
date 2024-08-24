#include <iostream>
#include <string>

#include "model.h"
#include "executor.h"
#include "threads.h"

void execute(ModelAction* action) {
    switch(action->get_type()) {
    case THREAD_START: 
        break;
    case PLACEHOLDER: {
        std::string* s = (std::string*)action->get_args();
        std::cout << "process " << model->get_scheduler()->get_process_id() << ", " << "thread "
            << model->get_scheduler()->get_thread_id() << ", " << *s << std::endl;
        model->get_placeholder_data().push_back(shared::string(s->c_str()));
        break;
    }
    case PTHREAD_CREATE: {
        struct pthread_params* params = (struct pthread_params*)action->get_args();
        *(int*)action->get_result() = model->get_scheduler()->new_thread(params->func, params->arg);
        break;
    }
    case PTHREAD_JOIN: {
        int tid = *(pthread_t*)action->get_args();
        Scheduler* scheduler = model->get_scheduler();
        Thread* thread = scheduler->get_thread(tid);

        if (thread->get_process_id() != process_id) {
            break;
        }

        printf("thread %d joining %d\n", thread_id, tid);
        while (thread->get_state() == THREAD_RUNNING) {
            scheduler->yield();
        }
        
        thread->free_stack(); 
        printf("%d joined %d completed\n", thread_id, tid);

        break;
    }
    case STORE: {
        model->get_scheduler()->get_thread(thread_id)->get_thread_memory()->addToStoreBuffer(action);
        break;
    }
    }
}
