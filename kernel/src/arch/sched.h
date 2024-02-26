#pragma once
#include <lib/auxv.h>
#include <sched/thread.h>

/**
 * @brief Destroys a thread
 * @warning Thread should not be on the scheduler queue when this is called,
 * set thread state to `THREAD_STATE_DESTROY` if you want to destroy a thread
 */
void arch_sched_thread_destroy(thread_t *thread);

/**
 * @brief Sets up a stack according to SysV ABI
 */
uintptr_t arch_sched_stack_setup(process_t *proc, char **argv, char **envp, auxv_t *auxv);

/**
 * @brief Creates a new userspace thread
 * @param ip essentially the entry point
 * @param sp userspace stack pointer
 */
thread_t *arch_sched_thread_create_user(process_t *proc, uintptr_t ip, uintptr_t sp);

/**
 * @brief Creates a new kernel thread
 * @param func thread function
 */
thread_t *arch_sched_thread_create_kernel(void (* func)());

/**
 * @brief Returns the active thread on the current CPU
 */
thread_t *arch_sched_thread_current();