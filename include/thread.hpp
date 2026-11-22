/* thread.hpp
 *
 * DESCRIPTION
 * Stackful user-space threads for the Arduino UNO, cooperative and
 * with statically allocated stacks.
 *
 * WHAT IT DOES
 * Every thread has its own stack and its own TCB, wrapped in the Thread<N> object
 * (N = stack size in bytes). thread_create prepares the thread and registers it in
 * the scheduler; the thread runs until it calls thread_yield(), which gives the CPU
 * to the next thread chosen by the scheduler (round robin, see scheduler.hpp). The
 * switch saves the registers and the stack pointer of the current thread and
 * restores those of the other one (context_switch, in assembly).
 * A new thread starts from entry_point, which calls the function passed to
 * thread_create; its stack is prepared at creation time by init_stack.
 *
 * LIMITS
 * - Cooperative only: there is no preemption, a thread keeps the CPU until it yields
 * - The stack size is fixed at compile time (minimum MIN_STACK_SIZE bytes)
 * - Thread objects must be static or global
 */





#pragma once

#include <stddef.h>
#include <stdint.h>

#include "scheduler.hpp"
#include "event.hpp"





constexpr uint8_t  STACK_FILL     = 0xAA;  // stack painter value
constexpr uint16_t MIN_STACK_SIZE = 64;    // bytes: initial frame (20) + room for calls and interrupts





/* READY:      the thread is in the READY list (it is running or waiting for its turn)
   BLOCKED:    the thread waits for an event, it is not in the READY list
   ZOMBIE:     the thread finished and nobody has read its retval yet (thread_join)
   TERMINATED: the thread finished and its retval was read: it is not used anymore */
enum ThreadState : uint8_t { READY, BLOCKED, ZOMBIE, TERMINATED };





/* Thread Control Block: everything the library needs to know about a thread.
   IMPORTANT: sp MUST stay the first field. context_switch (assembly) reads and writes
   it through the TCB address itself */
struct TCB {
   uint8_t *sp;                 // saved stack pointer (written at every context switch)
   TCB     *next;               // next thread in the READY list (circular) or in the finished list
   void    *args;               // argument passed to fn
   void    *retval;             // value returned by fn (valid once the thread is ZOMBIE or TERMINATED)
   void    *(*fn)(void*);       // function run by the thread

   volatile ThreadState state;  // volatile: thread_join re-reads it while another thread changes it
};





/* extern "C" disables name mangling, so the compiler looks for the plain symbol
   "context_switch", which is the one exported by .global in the .S file and shown to the linker */
extern "C" void context_switch(TCB *from, TCB *to);


extern TCB *running_thread;      // thread currently running
extern TCB  tcb_main;            // TCB of main (the thread running at startup)





void init_stack(TCB *tcb,  uint8_t *stack, size_t size,  void (*entry)());                      // prepare the stack of a new thread
void thread_join(TCB* tcb, void **retval);                                                      // wait for a thread to terminate and get its return value
void entry_point();                                                                             // first function run by every new thread





/* A thread with a statically allocated stack of N bytes (TCB + stack).
   The object only reserves the memory: the thread is initialized and registered
   by thread_create, and it is started later by the scheduler. */
template <uint16_t N>
struct Thread {
   static_assert(N >= MIN_STACK_SIZE, "Error: stack minimum size is 64 bytes");

   TCB     tcb;
   uint8_t stack[N];

   Thread() = default;

   /* stack painting: returns the maximum number of stack bytes used so far,
      to estimate how many bytes a thread really needs.
      Call it after thread_create (init_stack paints the stack).
      A created thread uses at least 20 bytes (the initial frame). */
   uint16_t stack_used() const {
      uint16_t free_bytes = 0;
      while (free_bytes < N && stack[free_bytes] == STACK_FILL) free_bytes++;
      return N - free_bytes;
   }
};





/* Initialize the TCB and the stack of "thread", then register it in the scheduler.
   It does not start the thread: it runs when the scheduler selects it.
   It is a template because the stack size N is part of the Thread<N> type.
   Returns the TCB, to be used with thread_join. Call it once per Thread. */
template <uint16_t N>
TCB *thread_create(Thread<N> *thread, void *(*fn)(void*), void *args) {
   TCB *tcb    = &thread->tcb;
   tcb->args   = args;
   tcb->fn     = fn;
   tcb->retval = nullptr;
   tcb->state  = READY;

   init_stack(tcb, thread->stack, N, entry_point);    
   thread_schedule(tcb);                              

   return tcb;
}