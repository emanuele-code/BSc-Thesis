#include <benchmark/benchmark.h>
#include <pthread.h>
#include <vector>



static void *dummy_function(void*) {
    return nullptr;
}



/* This function test the spawning and join of a batch of threads */
static void BM_pthread_spawnJoinBatch(benchmark::State& state) {
    const int              num_threads = static_cast<int>(state.range(0));
    std::vector<pthread_t> threads(num_threads);


    for(auto _ : state) {
        int created = 0;

        for(; created<num_threads; ++created) {
            if(pthread_create(&threads[created], nullptr, dummy_function, nullptr) != 0) {
                break;
            }
        }

        // join only the threads that were actually created
        for(int i=0; i<created; ++i) {
            pthread_join(threads[i], nullptr);
        }

        if(created != num_threads) {
            state.SkipWithError("pthread_create failed.");
            break;
        }
    }

    state.SetItemsProcessed(state.iterations()*num_threads); // show items_per_second = threads created_n_join/s as a field in csv
}




BENCHMARK(BM_pthread_spawnJoinBatch)
    ->RangeMultiplier(2)
    ->Range(1, 1024)
    ->UseRealTime()                       // measure the Wall-Clock and not the cpu time
    ->Unit(benchmark::kMicrosecond);      // use microseconds




/* Same test as above, but with N = 1..16 in steps of 1: used to find where the cost jump between N=4 and N=8 happens */
static void BM_pthread_spawnJoinDense(benchmark::State& state) {
    BM_pthread_spawnJoinBatch(state);
}



BENCHMARK(BM_pthread_spawnJoinDense)
    ->DenseRange(1, 16, 1)
    ->UseRealTime()
    ->Unit(benchmark::kMicrosecond);




/* Entry Point */
BENCHMARK_MAIN();