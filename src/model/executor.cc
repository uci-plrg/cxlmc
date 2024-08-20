#include <iostream>
#include <string>

#include "model.h"
#include "executor.h"
#include "threads.h"

void execute(ModelAction* action) {
    model->get_scheduler()->wait();
    switch(action->get_type()) {
    case PLACEHOLDER: {
        std::string* s = (std::string*)action->get_args();
        std::cout << "process " << model->get_scheduler()->get_process_id() << ", " << "thread "
            << model->get_scheduler()->get_thread_id() << ", " << *s << std::endl;
        model->get_user_data().push_back(shared::string(s->c_str()));
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
        thread_data_t* thread = scheduler->get_thread(tid);

        if (thread->process_id.load() != process_id) {
            break;
        }

        printf("thread %d joining %d\n", thread_id, tid);
        while (thread->state.load() == THREAD_RUNNING) {
            printf("%d still joining %d\n", thread_id, tid);
            scheduler->yield();
        }
        free(thread->stack);
        printf("%d joined %d completed\n", thread_id, tid);
        break;
    }
    }
}