#pragma once

struct TCB;

void thread_schedule(TCB *tcb);
void thread_yield();
