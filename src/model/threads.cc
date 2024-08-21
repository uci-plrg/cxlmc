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

void Thread::setup_tls() {
    // assert process id == thread id
    tls = (void*) get_tls_addr();
}

void* helper_thread(void* arg) {
    Thread* curr_thread = model->get_scheduler()->current_thread();

    real_pthread_mutex_lock(&curr_thread->mutex_tls);
    curr_thread->tls = (void*)get_tls_addr();
    real_pthread_mutex_unlock(&curr_thread->mutex_tls);
    real_pthread_mutex_lock(&curr_thread->mutex_finalize);
    real_pthread_mutex_unlock(&curr_thread->mutex_finalize);
    return nullptr;
}

void setup(void* (*func)(void*), void* arg) {
    model->action(new ModelAction(THREAD_START));
    Thread* curr_thread = model->get_scheduler()->current_thread();

    real_pthread_mutex_init(&curr_thread->mutex_tls, nullptr);
	real_pthread_mutex_init(&curr_thread->mutex_finalize, nullptr);
	real_pthread_mutex_lock(&curr_thread->mutex_finalize);

    real_pthread_create(&curr_thread->pthread_id, nullptr, helper_thread, nullptr);

    bool notdone = true;
	while(notdone) {
		real_pthread_mutex_lock(&curr_thread->mutex_tls);
		if (curr_thread->tls != nullptr)
			notdone = false;
		real_pthread_mutex_unlock(&curr_thread->mutex_tls);
	}
    set_tls_addr((uintptr_t)curr_thread->tls);
    curr_thread->ret_val = func(arg);

    real_pthread_join(curr_thread->pthread_id, nullptr);
    model->get_scheduler()->finalize();
    model->get_scheduler()->wait();
    printf("this should not be reached\n");
}

int Thread::setup_context(void* (*func)(void*), void* arg) {
    int ret;
    if ((ret = getcontext(&context)) != 0) {
        perror("getcontext");
        return ret;
    }

    context.uc_link = nullptr;
    context.uc_stack.ss_sp = stack = mspace_malloc(snapshot_space, STACK_SIZE);
    context.uc_stack.ss_size = STACK_SIZE;
    context.uc_stack.ss_flags = 0;
    makecontext(&context, (void(*)()) setup, 2, func, arg);
    return 0;
}

void Thread::swap(Thread* next) {
    if (next->tls)
        set_tls_addr((uintptr_t)next->tls);
    if (swapcontext(this->get_context(), next->get_context()) != 0) {
        perror("swapcontext");
    }
}