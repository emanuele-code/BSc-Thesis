/* thread.cc
 *
 * DESCRIPTION
 * Implementation of the stackful thread library declared in thread.hpp.
 *
 * WHAT IT DOES
 * Implements the functions of the thread library that do not need assembly:
 * entry_point (first function run by a new thread, it calls the thread function
 * and never returns), init_stack (paints the stack with STACK_FILL and writes the
 * initial frame, as if the thread had already been suspended), thread_join
 * (waits for a thread to finish and takes its return value) and thread_sleep
 * (waits for a number of milliseconds).
 * TCB of main, running_thread, thread_schedule and thread_yield are defined in
 * scheduler.cc
 *
 * LIMITS
 * - Works together with context_switch.S, which must be linked in and relies on
 *   the sp field being the first field of the TCB
 * - thread_join and thread_sleep wait by polling: they give the CPU to the others at
 *   every check, so they do not block them, but the waiting thread stays READY
 */



#include "thread.hpp"
#include "timer.hpp"





/* First function executed by a new thread.
   It is started by a ret (see init_stack), which cannot pass arguments,
   so it takes none: it reads fn and args from running_thread and calls fn.
   When fn returns it saves the result and marks the thread as ZOMBIE: the thread_yield
   that follows removes it from the READY list, so it never runs again. The final loop
   is only a safety net, because entry_point has no caller to return to. */
void entry_point() {
    running_thread->retval = running_thread->fn(running_thread->args);
    running_thread->state  = ZOMBIE;

    for(;;) thread_yield();
}





/* Prepare the stack as if the thread had already been suspended by context_switch:
   top: address of entry_point (read by the first ret), then 18 zeroed registers
   (the ones context_switch pops). tcb->sp points to the first free cell. */
void init_stack(TCB *tcb, uint8_t *stack, size_t size, void (*entry)(void)) {
    for(size_t i=0; i<size; i++) stack[i] = STACK_FILL;     // paint the whole stack (see stack_used)

    uint8_t *p    = stack + size - 1;                       // top of the stack: highest address
    uint16_t addr = (uint16_t)entry;                        // flash address of entry_point
    *p--          = addr & 0xFF;                            // low byte first, as the call instruction does
    *p--          = addr >> 8;                              // then the high byte

    for(size_t i=0; i<18; i++) *p-- = 0;                    // r2-r17, r28, r29 that context_switch will pop
    
    tcb->sp = p;                                            // first free cell
}





/* Wait for the thread "tcb" to terminate and copy its return value in *retval.
   It waits while the thread is READY or BLOCKED, giving the CPU to the other threads
   at every check, so it does not block them. If the thread is BLOCKED forever the join
   never returns. Calling it again on a TERMINATED thread returns at once with the same value.  */
void thread_join(TCB *tcb, void **retval) {
    while( tcb->state == READY || tcb->state == BLOCKED ) thread_yield();         
    if(retval) *retval = tcb->retval;
    tcb->state = TERMINATED;
}





/* Wait ms milliseconds giving the CPU to the other threads */
void thread_sleep(uint32_t ms) {
    uint32_t now = tick_now();
    while (tick_now() - now < ms) thread_yield();
}