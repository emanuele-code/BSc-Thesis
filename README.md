# BSc-Thesis

Bare-metal cooperative user-space threads for Arduino Uno (ATmega328P, 16 MHz), written in C++17 with avr-libc only (no Arduino core).

The project compares two kinds of threads on flash usage, RAM usage and context-switch cost:

- **Stackless**: protothreads, built on macros (`include/protothread.hpp`).
- **Stackful**: each thread has its own stack and the context switch is written in AVR assembly (`src/context_switch.S`).

## Project Structure

```text
.
├── include
│   ├── hal.hpp
│   ├── timer.hpp
│   ├── thread.hpp
│   ├── scheduler.hpp
│   ├── utilityPin.hpp
│   ├── protothread.hpp
│   ├── utilitySerial.hpp
│   └── conditionVariable.hpp
├── src
│   ├── hal.cc
│   ├── main.cc
│   ├── timer.cc
│   ├── thread.cc
│   ├── scheduler.cc
│   └── context_switch.S
├── docs
│   ├── atemega328p.pdf
│   └── stato_progetto.pdf
├── cmake
│   └── avr-g++.cmake
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Requirements

Ubuntu 24.04:

```bash
sudo apt update
sudo apt install build-essential cmake gcc-avr binutils-avr avr-libc avrdude
```

## Build

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/avr-g++.cmake
cmake --build build
```

If you change toolchain or compiler, delete `build/` first. The build produces `build/main.hex` and prints flash/RAM usage.

## Flash

Connect the board (it shows up as `/dev/ttyACM0`) and run:

```bash
avrdude -p atmega328p -c arduino -P /dev/ttyACM0 -b 115200 -U flash:w:build/main.hex:i
```

To read the UART output (9600 baud):

```bash
screen /dev/ttyACM0 9600
```

## User Interface (stackful threads)

Include `thread.hpp` (it also brings `scheduler.hpp` and `conditionVariable.hpp`) and `hal.hpp`.

### Program structure

```cpp
int main() {
    hal_init();                 // FIRST: starts the 1 ms tick, the UART and the interrupts
    // ... configure the pins, create the threads ...
    thread_start();             // LAST: main becomes the idle thread, it never returns
}
```

| Function | Description |
|---|---|
| `void hal_init()` | Starts the tick (Timer0, 1 ms), the UART0 (9600 baud) and enables the interrupts. Must be called before `thread_sleep` or `tick_now` are used |
| `void thread_start()` | Idle loop of `main`: it gives the CPU to the other threads forever. It never returns |

### Threads

| Function / type | Description |
|---|---|
| `Thread<N>` | Memory of one thread: TCB + stack of `N` bytes (`N >= 64`). Must be `static` or global |
| `TCB *thread_create(Thread<N> *t, void *(*fn)(void *), void *args)` | Prepares the thread and puts it in the READY list. It starts when the scheduler reaches it. Returns the TCB, to be used with `thread_join`. Call it once per `Thread` |
| `void thread_yield()` | Gives the CPU to the next READY thread (round robin) |
| `void thread_sleep(uint32_t ms)` | Waits at least `ms` milliseconds giving the CPU to the others (polling on the tick) |
| `void thread_join(TCB *t, void **retval)` | Waits for the thread to terminate and copies its return value in `*retval` (can be `nullptr`) |
| `uint16_t Thread<N>::stack_used()` | Maximum number of stack bytes used so far, to choose `N` |

Thread states: `READY` (running or waiting for its turn), `BLOCKED` (waiting on a `CondVar`),
`ZOMBIE` (finished, return value not read yet), `TERMINATED` (return value read).

### Condition variables (control synchronization)

Threads are cooperative, so nobody interrupts a thread in the middle of an operation: the data are already protected and no mutual exclusion is needed. What is still needed is a control synchronization, that is a way to wait until something happens ("the buffer is not empty", "the system is ready"). The condition variables (`conditionVariable.hpp`) provide it.

| Function / type | Description |
|---|---|
| `CondVar` | A condition to wait for. One `CondVar` per condition, `static` or global. Many threads can wait on or signal the same one |
| `void thread_wait(CondVar *cv)` | Blocks the calling thread (BLOCKED) until a signal or broadcast |
| `void thread_signal(CondVar *cv)` | Wakes the thread that waits since more time (FIFO), if any |
| `void thread_broadcast(CondVar *cv)` | Wakes all the threads that wait on `cv` |

Rules:

- The condition must always be checked in a loop: `while(!ready) thread_wait(&cv);`
- A signal or broadcast with nobody waiting is lost (a `CondVar` has no memory).
- No mutex is needed: threads are cooperative and nobody interrupts a thread between the check and the `thread_wait`.
- `main` must not wait: if it calls `thread_wait` it only yields.

### Time and hardware helpers

| Function / type | Description |
|---|---|
| `uint32_t tick_now()` (`timer.hpp`) | Milliseconds since `hal_init` (wraps after about 49.7 days) |
| `DP8` ... `DP13` (`utilityPin.hpp`) | Digital pins 8..13, resolved at compile time: `output()`, `input()`, `high()`, `low()`, `toggle()` |
| `uart_put(char)`, `uart_print_u16(uint16_t)` (`utilitySerial.hpp`) | Blocking transmit on UART0, for debugging |

### Example
 
A producer and a consumer share a buffer of 3 items. The producer is faster, so the buffer fills up and the producer has to wait.
 
```cpp
#include "thread.hpp"
#include "hal.hpp"
 
static CondVar not_full;                     // the producer waits here when the buffer is full
static CondVar not_empty;                    // the consumer waits here when the buffer is empty
static volatile uint8_t items = 0;           // produced and not yet consumed (0..3)
 
void *producer(void *) {
    for(;;) {
        while(items >= 3) thread_wait(&not_full);       // buffer full: BLOCKED
 
        DP13::toggle();                                 // pin 13 changes at every item produced
        thread_sleep(500);                              // time needed to produce
        items++;
        thread_signal(&not_empty);                      // wake up the consumer, if it waits
    }
    return nullptr;
}
 
void *consumer(void *) {
    for(;;) {
        while(items == 0) thread_wait(&not_empty);      // buffer empty: BLOCKED
 
        DP12::toggle();                                 // pin 12 changes at every item consumed
        thread_sleep(1200);                             // time needed to consume
        items--;
        thread_signal(&not_full);                       // wake up the producer, if it waits
    }
    return nullptr;
}
 
static Thread<64> t_producer, t_consumer;
 
int main() {
    hal_init();
    DP13::output();
    DP12::output();
 
    thread_create(&t_producer, producer, nullptr);
    thread_create(&t_consumer, consumer, nullptr);
 
    thread_start();                                     // never returns
}
```

## User Interface (protothreads)

Include `protothread.hpp`. A protothread is a function that receives a `Protothread` (2 bytes, the point where it resumes) and is called again and again by the program.

| Macro | Description |
|---|---|
| `PT_INIT(pt)` | Initializes the protothread: use it right after its creation |
| `PT_BEGIN(pt)` / `PT_END(pt)` | Open and close the protothread body (`PT_END` returns `PT_ENDED`) |
| `PT_WAIT_UNTIL(pt, cond)` | Returns `PT_WAITED` until `cond` is true |
| `PT_YIELD(pt)` | Returns `PT_YIELDED` and resumes after the yield at the next call |
| `PT_EXIT(pt)` | Ends the protothread and returns `PT_EXITED` |

Limits: local variables are not kept across a wait or yield (keep them in a struct), `switch` statements inside a protothread are not allowed, and each macro must be on a different source line.