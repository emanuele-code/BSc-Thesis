/* protothread.h
 *
 * DESCRIPTION
 * Minimal protothread implementation: stackless, cooperative
 * threads written as plain C++ macros 
 *
 * WHAT IT DOES
 * Lets a function suspend itself and later resume from the same point, so
 * that sequences with waits can be written in a linear, thread-like style.
 * The only state kept between calls is a 2-byte local continuation that state 
 * where to resume the function. 
 * 
 * LIMITS
 * - Using protothread might invalidate the use of a switch-case 
 *   statment as they rely on them
 * - Local variables are not preserved across a wait or yields, 
 *   so the programmer must keep them in a context struct
 * 
 * NOTE
 * This header can be extended as more protothread functions are needed
 * 
 */



#pragma once

#include <stdint.h>


/* local continuation: It tells where the protothread stopped */ 
struct Protothread { uint16_t lc; };



#define PT_WAITED  0
#define PT_YIELDED 1
#define PT_EXITED  2
#define PT_ENDED   3




/* init the local continuation: must be used right after the protothread creation */
#define PT_INIT(pt) ( (pt)->lc = 0)


/* Open the switch-case and is used to decide where to jump */
#define PT_BEGIN(pt) switch((pt)->lc) { case 0:


/* Saves the local continuation and leaves a case label 
 * so that the next call resumes right after this point. */
#define LC_SET(pt) (pt)->lc = __LINE__; [[fallthrough]]; case __LINE__:


/* Waits until the condition becomes true: while it is false, the function
 * returns PT_WAITED and the next call re-evaluates it from this point. */
#define PT_WAIT_UNTIL(pt, cond)  \
    do {                         \
        LC_SET(pt);              \
        if( !(cond) ) {          \
            return PT_WAITED;    \
        }                        \
    } while(0)                


/* Yield voluntarily: saves the resume point, returns PT_YIELDED and leaves a
 * case label so that the next call continues right after the yield. */
#define PT_YIELD(pt)                                             \
    do {                                                         \
        (pt)->lc = __LINE__; return PT_YIELDED; case __LINE__:;  \
    } while(0)         


/* Close the switch statment, restore the local continuation to 0 and return PT_ENDED */
#define PT_END(pt) } PT_INIT(pt); return PT_ENDED


/* Exit and restore the local continuation to 0 */
#define PT_EXIT(pt)        \
    do {                   \
        PT_INIT(pt);       \
        return PT_EXITED;  \
    } while(0)
    