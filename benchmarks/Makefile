CXX        = g++
CXXFLAGS   = -O2 -std=c++20 -Wall -Wextra
BENCH_ARGS = --benchmark_repetitions=30 --benchmark_report_aggregates_only=true


all: bench_spawnJoin


bench_spawnJoin: bench_spawnJoin.o
	$(CXX) bench_spawnJoin.o -o bench_spawnJoin -lbenchmark -pthread


bench_spawnJoin.o: bench_spawnJoin.cc
	$(CXX) $(CXXFLAGS) -pthread -c bench_spawnJoin.cc -o bench_spawnJoin.o


# Runs BM_pthread_spawnJoinBatch: N = 1, 2, 4, 8, ..., 1024, default stack cache (40 MiB).
run: bench_spawnJoin
	taskset -c 3 ./bench_spawnJoin $(BENCH_ARGS) --benchmark_filter=spawnJoinBatch --benchmark_out=results/bench_spawnJoin.csv --benchmark_out_format=csv


# Runs BM_pthread_spawnJoinDense: N = 1, 2, 3, ..., 16, default stack cache (40 MiB).
run_dense: bench_spawnJoin
	taskset -c 3 ./bench_spawnJoin $(BENCH_ARGS) --benchmark_filter=spawnJoinDense --benchmark_out=results/bench_spawnJoin_dense.csv --benchmark_out_format=csv


# Runs BM_pthread_spawnJoinDense: N = 1, 2, 3, ..., 16, stack cache raised to 1 GiB.
run_dense_bigcache: bench_spawnJoin
	GLIBC_TUNABLES=glibc.pthread.stack_cache_size=1073741824 taskset -c 3 ./bench_spawnJoin $(BENCH_ARGS) --benchmark_filter=spawnJoinDense --benchmark_out=results/bench_spawnJoin_dense_bigcache.csv --benchmark_out_format=csv


# Runs BM_pthread_spawnJoinDense: N = 1, 2, 3, ..., 16, stack cache disabled (0).
run_dense_nocache: bench_spawnJoin
	GLIBC_TUNABLES=glibc.pthread.stack_cache_size=0 taskset -c 3 ./bench_spawnJoin $(BENCH_ARGS) --benchmark_filter=spawnJoinDense --benchmark_out=results/bench_spawnJoin_dense_nocache.csv --benchmark_out_format=csv


# Runs the three BM_pthread_spawnJoinDense measurements in sequence: 40 MiB, 1 GiB and 0 cache.
run_cache_test: run_dense run_dense_bigcache run_dense_nocache


clean:
	rm -f bench_spawnJoin bench_spawnJoin.o


.PHONY: all run run_dense run_dense_bigcache run_dense_nocache run_cache_test clean