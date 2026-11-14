/* context_switch.S
 *
 * SIGNATURE
 * void context_switch(TCB *from, TCB *to)
 *
 * DESCRIPTION
 * context switch 2 threads
 *
 * WHAT IT DOES
 * Switches the CPU from the thread that is yielding (from) to the thread that
 * gains the CPU (to). The state of a thread (callee-saved registers and return
 * address) lives on its own stack, so switching means: save the registers of
 * the thread that leaves, swap the stack pointer, restore the registers of the
 * thread that enters and let ret load the new PC from the new stack.
 *
 * Layout assumed for the TCB: the first 2 bytes are the saved SP (low byte first)
 *
 *
 * REGISTERS:
 *   CALLEE-SAVED -> must be saved/restored if used
 *      r2-r17 and r28:r29 (Y, the frame pointer)
 *   CALL-USED    -> can be trashed
 *      r0              temporary register (SPL, SPH and SREG pass through it)
 *      r18-r19         hold to->sp: its value is then written in the SP register
 *      r22:r23         argument "to": pointer to the TCB of the thread that gains the CPU
 *      r24:r25         argument "from": pointer to the TCB of the thread that is yielding
 *      r30:r31         (Z) pointer used by st and ld to access the TCB in RAM
 *                      (it is also used by the compiler for indirect calls to functions)
 *   NOT USED
 *      r1              always 0 (avr-gcc convention)
 */







#include <avr/io.h>                    

        .section .text                  ; code section (ends up in flash)
        .global context_switch          ; visible to the linker (called from C++ with extern "C")

context_switch:
        


        ; ---- 1) FROM thread: save its callee-saved registers on its own stack ----
        ;      it expects to find them unchanged once it gets the CPU back 

        push r2                          
        push r3                           
        push r4                         
        push r5
        push r6
        push r7
        push r8
        push r9
        push r10
        push r11
        push r12
        push r13
        push r14
        push r15
        push r16
        push r17
        push r28                        ; r28:r29 = Y, the frame pointer when the caller has a stack
        push r29                        ; frame (many locals, arrays, &var); a normal callee-saved
                                        ; register otherwise. Saved because the caller expects it unchanged.



        ; ---- 2) from->sp = SP (the TCB field is a bookmark: it is rewritten at every switch) ----

        movw r30, r24                   ; Z = from (r25:r24 -> r31:r30), Z points to the start of the TCB
        in r0, _SFR_IO_ADDR(SPL)        ; r0 = low byte of the stack pointer (SPL is an I/O register)
        st Z+, r0                       ; low half of from->sp = r0 then move to the next byte (Z++)
        in r0, _SFR_IO_ADDR(SPH)        ; r0 = high byte of the stack pointer
        st Z, r0                        ; high half of from->sp = r0



        ; ---- 3) read to->sp into r19:r18 ---- 

        movw r30, r22                   ; Z = to (r23:r22 -> r31:r30) 
        ld r18, Z+                      ; r18 = low half of to->sp (read from RAM) then move to the next byte
        ld r19, Z                       ; r19 = high half of to->sp



        ; ---- 4) SP = to->sp: from here the CPU works on the stack of the TO thread ----
        ;      SP is written in two steps, so no interrupt must happen in the middle

        in   r0, _SFR_IO_ADDR(SREG)     ; save SREG 
        cli                             ; disable interrupts
        out  _SFR_IO_ADDR(SPH), r19     ; SPH = high byte of to->sp
        out  _SFR_IO_ADDR(SREG), r0     ; restore SREG: I returns to its previous value; the next
                                        ; instruction runs before any pending interrupt is taken
        out  _SFR_IO_ADDR(SPL), r18     ; SPL = low byte of to->sp: SP is now complete



        ; ---- 5) TO thread: restore its registers (reverse order of the pushes) ----
        ;      pop = SP++, then register = the byte pointed by SP

        pop r29                         
        pop r28                         
        pop r17                         
        pop r16
        pop r15
        pop r14
        pop r13
        pop r12
        pop r11
        pop r10
        pop r9
        pop r8
        pop r7
        pop r6
        pop r5
        pop r4
        pop r3
        pop r2



        ; ---- 6) resume the TO thread ----
        ;      SP += 2 and PC = the 2 bytes on top of the stack (a flash address):
        ;      new thread:         the address of entry_point written by init_stack
        ;      thread already run: the return address wrtten by the last call to yield(), saved when it was suspended

        ret