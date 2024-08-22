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
        break;
    case THREAD_FINISH: {
        model->get_scheduler()->wake_threads_waiting_on(get_thread(action));
        break;
    }
    case PLACEHOLDER: {
        std::string* s = (std::string*)action->get_location();
        std::cout << "process " << model->get_scheduler()->get_process_id() << ", " << "thread "
            << model->get_scheduler()->get_thread_id() << ", " << *s << std::endl;
        model->get_user_data().push_back(shared::string(s->c_str()));
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

        if (thread->get_process_id() != process_id) {
            // error? waiting on a thread of another process
            break;
        }

        printf("thread %d joining %d\n", thread_id, thread->get_thread_id());
        if (thread->get_state() != THREAD_COMPLETED) {
            curr_thread->set_state(THREAD_BLOCKED);
            model->get_scheduler()->yield();
        }
        thread->free_stack();
        printf("%d joined %d completed\n", thread_id, thread->get_thread_id());
        break;
    }
    }
}
