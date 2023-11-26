add_executable(lru-list-benchmark
        lru-tbb-benchmark.cpp
        lru-bench-util.cpp)

target_link_libraries_system(lru-list-benchmark
        benchmark::benchmark
        benchmark::benchmark_main
        absl::random_random)

target_link_libraries(lru-list-benchmark PUBLIC dcds)

target_compile_features(lru-list-benchmark PUBLIC cxx_std_20)

dcds_target_enable_default_warnings(lru-list-benchmark)
