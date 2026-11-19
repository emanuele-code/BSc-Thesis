/* thread.cc
 *
 * DESCRIPTION
 * Implementation of the stackful thread library declared in thread.hpp.
 *
 * WHAT IT DOES
 * Defines the TCB of main and the pointer to the running thread, and implements the
 * three functions that do not need assembly: yield (updates running_thread and jumps
 * to context_switch), entry_point (first function run by a new thread, it calls the
 * thread function and never returns) and init_stack (paints the stack with STACK_FILL
 * and writes the initial frame, as if the thread had already been suspended).
 *
 * LIMITS
 * Works together with context_switch.S, which must be linked in and relies on
 * the sp field being the first field of the TCB.
 *
 * NOTE
 * many of the implementation are subjected to the absence of the scheduler
 * thus, function such as yield(TCB* next) will be refactored after
 */



#include "thread.hpp"



TCB  tcb_main;                        // TCB of main: its sp is written at the first context-switch
TCB  *running_thread = &tcb_main;     // This is the thread running in that moment


/* Voluntarily give the CPU to another thread.
   Only updates running_thread and jumps to context_switch (tail call):
   the return address on the stack is the one written by the caller's call. */
void yield(TCB *next) {
    TCB *prev      = running_thread;
    running_thread = next;
    context_switch(prev, next);
}



/* First function executed by a new thread.
   It is started by a ret (see init_stack), which cannot pass arguments,
   so it takes none: it reads fn and args from running_thread and calls fn.
   When fn returns it saves the result and never runs again. */
void entry_point() {
    running_thread->retval = running_thread->fn(running_thread->args);
    running_thread->done   = true;

    for(;;) yield(&tcb_main);
}




void thread_join(TCB* tcb, void **retval) {
    while(tcb->done == false) yield(tcb);
    *retval = tcb->retval;
}



/* Prepare the stack as if the thread had already been suspended by context_switch:
   top: address of entry_point (read by the first ret), then 18 zeroed registers
   (the ones context_switch pops). tcb->sp points to the first free cell. */
void init_stack(TCB *tcb, uint8_t *stack, size_t size, void (*entry)(void)) {
    for(size_t i=0; i<size; i++) stack[i] = STACK_FILL;

    uint8_t *p    = stack + size - 1;
    uint16_t addr = (uint16_t)entry;
    *p--          = addr & 0xFF;
    *p--          = addr >> 8;

    for(size_t i=0; i<18; i++) *p-- = 0;
    
    tcb->sp = p;
}
