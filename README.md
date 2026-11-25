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

Connect the board (it shows up as `/dev/ttyACM0`; boards with a CH340 USB chip, like many Arduino Uno clones, show up as `/dev/ttyUSB0`) and run:

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
| `TCB *thread_create(Thread<N> *t, void *(*fn)(void *), void *args)` | Prepares the thread and puts it in the READY list. It starts when the scheduler reaches it. Returns the TCB, to be used with `thread_join` or `thread_detach`, or `nullptr` if the `Thread` is still in use (see "Reusing a Thread") |
| `void thread_yield()` | Gives the CPU to the next READY thread (round robin) |
| `void thread_sleep(uint32_t ms)` | Waits at least `ms` milliseconds giving the CPU to the others (polling on the tick) |
| `void thread_join(TCB *t, void **retval)` | Waits for the thread to terminate and copies its return value in `*retval` (can be `nullptr`). Returns at once if `t` is `nullptr`. Do not use it on a detached thread |
| `void thread_detach(TCB *t)` | Nobody will join the thread: when it finishes it goes straight to `TERMINATED` and its return value is discarded. On a thread that is already `ZOMBIE` it makes it `TERMINATED` at once. Does nothing if `t` is `nullptr` |
| `uint16_t Thread<N>::stack_used()` | Maximum number of stack bytes used so far, to choose `N` |

### Thread states

| State | Meaning |
|---|---|
| `TERMINATED` | The `Thread` is not in use: it was never created, or it finished and was joined, or it finished and was detached. It can be created again |
| `READY` | The thread is in the READY list (it is running or waiting for its turn) |
| `BLOCKED` | The thread waits on a `CondVar`, it is not in the READY list |
| `ZOMBIE` | The thread finished and nobody has read its return value yet |

A new `Thread` (static or global) starts as `TERMINATED`. Transitions:

| From | To | When |
|---|---|---|
| `TERMINATED` | `READY` | `thread_create` |
| `READY` | `BLOCKED` | the thread calls `thread_wait` |
| `BLOCKED` | `READY` | another thread calls `thread_signal` or `thread_broadcast` |
| `READY` | `ZOMBIE` | the thread function returns |
| `READY` | `TERMINATED` | the thread function returns and the thread is detached |
| `ZOMBIE` | `TERMINATED` | `thread_join` (or `thread_detach`) |

### Reusing a Thread

A `Thread` can be created again as soon as it is `TERMINATED`: the stack is painted again and the thread starts from the beginning. If it is still in use (`READY`, `BLOCKED` or `ZOMBIE`) nothing is modified and `thread_create` returns `nullptr`.

A thread that nobody joins can be detached, so its `Thread` is free again as soon as it finishes:

```cpp
static Thread<64> t_blink;

void *blink3(void *) {
    for(int i = 0; i < 3; i++) { DP13::toggle(); thread_sleep(200); }
    return nullptr;
}

// every time the activity is needed (for example at a button press):
TCB *t = thread_create(&t_blink, blink3, nullptr);   // nullptr if the previous run is still going
thread_detach(t);                                    // safe with nullptr; the Thread is free when blink3 ends
```

The function does not have to be the same: the same `Thread` can run `blinker<DP7>` now and `blinker<DP9>` the next time, so one stack serves different activities one after the other. A few rules:

- Only a thread that returns can be reused. A thread with an endless loop (`for(;;)`) never becomes `TERMINATED`.
- `N` must be enough for the most demanding function that runs on the `Thread`.
- The stack is painted again at every `thread_create`: what must survive between two runs (for example the position of a motor) has to be global or `static`, not a local variable.

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
| `DP2` ... `DP13` (`utilityPin.hpp`) | Digital pins 2..13 (port D: 2..7, port B: 8..13), resolved at compile time: `output()`, `input()`, `high()`, `low()`, `toggle()`, `read()` (true if the pin is high). Pins 0 and 1 are the UART and are not available; the analog pins A0..A5 are not supported |
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

Limits: local variables are not kept across a wait or yield (keep them in a struct), `switch` statements inside a protothread are not allowed, each macro must be on a different source line, and no variable with an initializer can be declared between `PT_BEGIN` and `PT_END`.