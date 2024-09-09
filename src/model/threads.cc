#include "threads.h"
#include "model.h"

uintptr_t get_tls_addr() {
	uintptr_t addr;
	asm ("mov %%fs:0, %0" : "=r" (addr));
	return addr;
}

#include <asm/prctl.h>
#include <sys/prctl.h>
extern "C" {
int arch_prctl(int code, unsigned long addr);
}
static void set_tls_addr(uintptr_t addr) {
	arch_prctl(ARCH_SET_FS, addr);
	asm ("mov %0, %%fs:0" : : "r" (addr) : "memory");
}

void* helper_thread(void* arg) {
    Thread* thread = (Thread*) arg;

    real_pthread_mutex_lock(&thread->mutex_tls);
    thread->tls = (void*)get_tls_addr();
    real_pthread_mutex_unlock(&thread->mutex_tls);
    real_pthread_mutex_lock(&thread->mutex_finalize);
    real_pthread_mutex_unlock(&thread->mutex_finalize);
    return nullptr;
}

void thread_start() {
    model->action(new ModelAction(THREAD_START));
    Thread* curr_thread = model->get_scheduler()->current_thread();
    pthread_params params = curr_thread->params;
    curr_thread->ret_val = params.func(params.arg);
    model->action(new ModelAction(THREAD_FINISH)); // does not return
}

int Thread::setup_context() {
    int ret;
    if ((ret = getcontext(&context)) != 0) {
        perror("getcontext");
        return ret;
    }

    context.uc_link = nullptr;
    context.uc_stack.ss_sp = stack = mspace_malloc(snapshot_space, STACK_SIZE);
    context.uc_stack.ss_size = STACK_SIZE;
    context.uc_stack.ss_flags = 0;

    real_pthread_mutex_init(&mutex_tls, nullptr);
	real_pthread_mutex_init(&mutex_finalize, nullptr);
	real_pthread_mutex_lock(&mutex_finalize);

    real_pthread_create(&pthread_id, nullptr, helper_thread, (void*)this);

    bool notdone = true;
	while(notdone) {
		real_pthread_mutex_lock(&mutex_tls);
		if (tls != nullptr)
			notdone = false;
		real_pthread_mutex_unlock(&mutex_tls);
	}

    makecontext(&context, thread_start, 0);
    return 0;
}

void Thread::swap(Thread* next) {
    if (!tls)
        tls = (void*) get_tls_addr();
    set_tls_addr((uintptr_t)next->tls);
    if (swapcontext(this->get_context(), next->get_context()) != 0) {
        perror("swapcontext");
    }
}

void Thread::cleanup() {
    real_pthread_mutex_unlock(&mutex_finalize);
    real_pthread_join(pthread_id, nullptr);
    mspace_free(snapshot_space, stack);
}

void Thread::finalize() {
    cleanup();
    model->get_scheduler()->finalize();
    model->get_scheduler()->wait();
    assert(0);
}

Thread* Thread::waiting_on() {
    if (pending && pending->get_type() == PTHREAD_JOIN) {
        return (Thread*)pending->get_location();
    }
    return nullptr;
}

Thread::Thread(thread_id_t tid, process_id_t pid, Thread* par, pthread_params p) :
    thread_id(tid),
    process_id(pid),
    state(THREAD_RUNNING),
    parent(par),
    is_main(false),
    stack(nullptr),
    params(p),
    tls(nullptr) {
        setup_context();
    }

Thread::Thread(process_id_t pid) :
    thread_id(pid),
    process_id(pid),
    state(THREAD_RUNNING),
    parent(nullptr),
    is_main(true),
    stack(nullptr),
    tls(nullptr) {
    }
