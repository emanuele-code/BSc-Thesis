CXX = g++
CXXFLAGS = -std=c++20 -O3 
LDLIBS = -lbenchmark -lpthread

all: bench_pthread run

bench_pthread: bench_pthread.o
	$(CXX) bench_pthread.o -o bench_pthread $(LDLIBS)

bench_pthread.o: bench_pthread.cc
	$(CXX) $(CXXFLAGS) -c bench_pthread.cc -o bench_pthread.o

run: bench_pthread
	taskset -c 0 ./bench_pthread --benchmark_repetitions=30 --benchmark_report_aggregates_only=true --benchmark_format=csv > pthread_baseline.csv

clean: 
	rm -f bench_pthread bench_pthread.o 
