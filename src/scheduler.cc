/* scheduler.cc
 *
 * DESCRIPTION
 * Round robin scheduler for the stackful threads declared in thread.hpp.
 *
 * WHAT IT DOES
 * The READY threads are kept in a circular linked list built with the "next" field of
 * the TCB (no extra memory, no maximum number of threads). The list always contains
 * main (tcb_main) and every new thread is appended at the end by thread_schedule,
 * which is called by thread_create. thread_yield gives the CPU to the next thread
 * of the list, so the threads run one after the other in a circle. A thread that
 * finishes is removed from the list in O(1) and moved to the "finished" list, so
 * it is never visited again. There is no scheduler object to create: it starts
 * working at the first thread_yield() of main.
 *
 * LIMITS
 * - Cooperative only: a thread keeps the CPU until it calls thread_yield
 * - Only the running thread can leave the READY list: the O(1) removal relies on
 *   running_prev, the predecessor of the running thread
 * - main is never removed: it must stay READY and works as the idle thread
 */



#include "thread.hpp"





/* TCB of main: it is the first element of the list, and its "next" points to itself,
   so at the beginning the list is made only of main. Its sp is written at the first
   context switch. The fields are: sp, next, args, retval, fn, state */
TCB tcb_main        = {nullptr, &tcb_main, nullptr, nullptr, nullptr, READY};
TCB *running_thread = &tcb_main;    





/* running_prev: predecessor of the running thread in the READY list, it lets
                 thread_yield unlink the running thread in O(1)
   tail:         last thread of the READY list (the one that points to main)
   finished:     threads that terminated, linked through "next" and ended by
                 nullptr: the scheduler never visits them again */
static TCB *running_prev = &tcb_main;
static TCB *tail         = &tcb_main; 
static TCB *finished     = nullptr;                                                    





/* Add a new thread at the end of the list.
   The new thread points to main to keep the list circular, then the old last
   thread points to the new one, which becomes the last. */
void thread_schedule(TCB *tcb) {
    tcb->next  = &tcb_main;
    tail->next = tcb;
    tail       = tcb;
}





/* Voluntarily give the CPU to the next thread of the READY list (round robin).
   If the running thread is still READY it stays in the list and becomes the new
   running_prev. Otherwise (it finished) it is unlinked in O(1) using running_prev
   and moved to the "finished" list.
   If main is the only thread in the list there is nothing to switch, so it just returns. */
void thread_yield() {
    TCB *self = running_thread;
    TCB *next = self->next; 

    if( self->state == READY ) {
        running_prev = self;
    } else {
        running_prev->next = self->next;
        if(tail == self) tail = running_prev;
        self->next = finished;
        finished   = self;
    }

    if(next == self) return;

    running_thread = next;
    context_switch(self, next);                         
}





void thread_block(TCB **wait_list) {
    TCB *self = running_thread;
    TCB *next = self->next;

    self->state = BLOCKED;

    running_prev->next = self->next;

    if( tail == self ) tail = running_prev;

    self->next = *wait_list;
    *wait_list   = self;

    running_thread = next;
    context_switch(self, next);
}





void thread_wake_all(TCB **wait_list) {
    while( *wait_list != nullptr ) {
        TCB *tcb   = *wait_list;
        *wait_list = tcb->next;
        tcb->state = READY;
        thread_schedule(tcb);
    } 
}





/* Idle loop of main: it keeps giving the CPU to the other threads and never returns.
   Called by start_system, after the threads have been created. */
void thread_start() {
    for(;;) thread_yield();     // idle: give the CPU to the other threads, forever
}