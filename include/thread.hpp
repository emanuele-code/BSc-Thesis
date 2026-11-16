/* thread.hpp
 *
 * DESCRIPTION
 * Stackful user-space threads for the Arduino UNO (ATmega328P), cooperative and
 * with statically allocated stacks.
 *
 * WHAT IT DOES
 * Every thread has its own stack and its own TCB, wrapped in the Thread<N> object
 * (N = stack size in bytes). A thread runs until it calls yield(next), which gives
 * the CPU to "next" by saving the registers and the stack pointer of the current
 * thread and restoring those of the other one (context_switch, in assembly).
 * A new thread starts from entry_point, which calls the function passed to the
 * constructor, so its stack is prepared at construction time by init_stack.
 *
 * LIMITS
 * - Cooperative only: there is no preemption, and the next thread is always chosen
 *   explicitly by the caller of yield (there is no scheduler yet).
 * - The stack size is fixed at compile time (minimum MIN_STACK_SIZE bytes)
 * - context_switch depend on the AVR architecture
 */



#pragma once

#include <stddef.h>
#include <stdint.h>



constexpr uint8_t  STACK_FILL     = 0xAA;  // stack painter value
constexpr uint16_t MIN_STACK_SIZE = 64;    // bytes: initial frame (20) + room 




/* Thread Control Block: everything the library needs to know about a thread.
   IMPORTANT: sp MUST stay the first field. context_switch (assembly) reads and writes
   it through the TCB address itself */
struct TCB {
    uint8_t *sp;                 
    void    *args;               
    void    *retval;             
    void    *(*fn)(void*);       // function run by the thread
};




/* extern "C" disables name mangling, so the compiler looks for the plain symbol
   "context_switch", which is the one exported by .global in the .S file and shown to the linker */
extern "C" void context_switch(TCB *from, TCB *to);




extern TCB *running_thread;      // thread currently running
extern TCB  tcb_main;            // TCB of main (the thread running at startup)




void init_stack(TCB *tcb,  uint8_t *stack, size_t size,  void (*entry)());  // prepare the stack of a new thread
void yield(TCB *next);                                                      // give the CPU to "next" (will be modified)
void entry_point();                                                         // first function run by every new thread




/* A thread with a statically allocated stack of N bytes.
   The constructor fills the TCB and prepares the stack (it does not start the thread, ret in assembly will). */
template <uint16_t N>
struct Thread {
    static_assert(N >= MIN_STACK_SIZE, "Error: stack minimum size is 64 bytes");

    TCB     tcb;
    uint8_t stack[N];

    Thread(void *(*fn)(void*), void *args) {
        tcb.args   = args;
        tcb.fn     = fn;
        tcb.retval = nullptr;

        init_stack(&tcb, stack, N, entry_point);
    }


    /* stack painting: returns the maximum number of stack bytes used so far,
       to estimate how many bytes a thread really needs. 
       when a thread is constructed it use at least 20 bytes */
    uint16_t stack_used() const {
        uint16_t free_bytes = 0;
        while (free_bytes < N && stack[free_bytes] == STACK_FILL) free_bytes++;
        return N - free_bytes;
    }
};