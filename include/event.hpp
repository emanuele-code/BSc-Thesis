#pragma once

#include "scheduler.hpp"

struct ECB { TCB *wait_list = nullptr; };

inline void thread_wait(ECB *ecb)      { thread_block(&ecb->wait_list);    }
inline void thread_broadcast(ECB* ecb) { thread_wake_all(&ecb->wait_list); }