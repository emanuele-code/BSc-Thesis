/* scheduler.cc
 *
 * DESCRIPTION
 * Round robin scheduler for the stackful threads declared in thread.hpp, with the
 * primitives to block a thread and wake it up (used by the CondVar of conditionVariable.hpp).
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
 * A thread that waits for an event is BLOCKED: thread_block removes it from the READY
 * list and links it, through the same "next" field, in the wait list of the event (a
 * thread is in one list at a time). thread_wake_all / thread_wake_one move it back to
 * the end of the READY list.
 *
 * LIMITS
 * - Cooperative only: a thread keeps the CPU until it calls thread_yield or thread_block
 * - Only the running thread can leave the READY list: the O(1) removal relies on
 *   running_prev, the predecessor of the running thread
 * - main is never removed: it must stay READY and works as the idle thread, so if it
 *   calls thread_block it does not block, it only yields
 * - thread_block, thread_wake_all and thread_wake_one modify the READY list: they must
 *   not be called from an interrupt
 */



#include "scheduler.hpp"
#include "thread.hpp"





/* TCB of main: it is the first element of the list, and its "next" points to itself,
   so at the beginning the list is made only of main. Its sp is written at the first
   context switch. The fields are: sp, next, args, retval, fn, state */
TCB tcb_main        = {nullptr, &tcb_main, nullptr, nullptr, nullptr, READY};
TCB *running_thread = &tcb_main;    





/* running_prev: predecessor of the running thread in the READY list, it lets
                 thread_yield and thread_block unlink the running thread in O(1)
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





/* Block the running thread on the wait list of an event.
   wait_list is the address of the head of that list.
   If main calls it the thread is not blocked: it only yields (main is the idle thread
   and must stay in the READY list), as a spurious wakeup.
   Steps, in this order:
     1. the thread becomes BLOCKED
     2. it is unlinked from the READY list in O(1) using running_prev (and tail is moved
        back if it was the last one)
     3. only now its "next" field is free to be reused: the thread is pushed at the
        head of the wait list
     4. the CPU goes to the next READY thread.
   running_prev does not change: the next thread has the same predecessor as the one
   that was just removed.
   When the thread is woken up it continues right after the context_switch. */
void thread_block(TCB **wait_list) {
    if(running_thread == &tcb_main) {
        thread_yield();
        return;
    }

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





/* Wake up every thread of the wait list (broadcast).
   Each thread becomes READY and is appended at the end of the READY list. The head of
   the wait list is moved forward BEFORE thread_schedule, because thread_schedule
   overwrites the "next" field of the thread. The woken threads do not run now: they
   run when the round robin reaches them. At the end the wait list is empty. */
void thread_wake_all(TCB **wait_list) {
    while( *wait_list != nullptr ) {
        TCB *tcb   = *wait_list;
        *wait_list = tcb->next;
        tcb->state = READY;
        thread_schedule(tcb);
    } 
}





/* Wake up only ONE thread of the wait list: the one that waits since more time (FIFO).
   thread_block pushes at the head, so the oldest waiter is the LAST element: the loop
   goes to the end of the list keeping its predecessor (prev). The last element is
   then removed from the list (if it was the only one, the list becomes empty), made READY
   and moved to the READY list.
   If nobody is waiting nothing happens: the signal is lost. */
void thread_wake_one(TCB **wait_list) {
    if( *wait_list == nullptr ) return;

    TCB *prev = nullptr;
    TCB *tcb  = *wait_list;
    while( tcb->next != nullptr ) {
        prev = tcb;
        tcb  = tcb->next;
    }

    if(prev != nullptr) {
        prev->next = nullptr;
    } else {
        *wait_list = nullptr;
    }

    tcb->state = READY;
    thread_schedule(tcb);
}





/* Idle loop of main: it keeps giving the CPU to the other threads and never returns.
   Called by main as its last instruction, after hal_init and after the threads
   have been created. */
void thread_start() {
    for(;;) thread_yield();     // idle: give the CPU to the other threads, forever
}