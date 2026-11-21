#pragma once

#include "hal.hpp"
#include "thread.hpp"

/* Initialize the hardware and start the scheduler.
   Call it once, at the end of main, after the threads have been created. */
inline void start_system() {
    hal_init();
    thread_start();
}