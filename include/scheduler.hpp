#pragma once

struct TCB;

void thread_wake_all(TCB **wait_list);
void thread_block(TCB **wait_list);
void thread_schedule(TCB *tcb);
void thread_yield();
void thread_start();