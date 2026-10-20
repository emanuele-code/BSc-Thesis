#include <benchmark/benchmark.h>
#include <sched.h>

/* This benchmark measures the overhead of the Linux kernel scheduler (Context Switching)
   google benchmark 
 */
 
static void BM_KernelSchedulerYield(benchmark::State& state) {
    const size_t yields_per_thread = 1000;

    for (auto _ : state) {
        for (size_t i = 0; i < yields_per_thread; ++i) {
            sched_yield(); 
        }
    }
    
    state.SetItemsProcessed(state.iterations() * yields_per_thread);
}


/*
 * ThreadRange(2, 1024): Executes the benchmark with 2 concurrent threads, then 4, 8... up to 1024.
 * UseRealTime(): it measure the wall-clock time from when the thread start until the last one finishes,
 				  rather than summing up the individual CPU times of all threads
 * Unit(): Displays the output in milliseconds for better readability.
 */
BENCHMARK(BM_KernelSchedulerYield)
    ->ThreadRange(2, 1024)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime(); 

BENCHMARK_MAIN();
