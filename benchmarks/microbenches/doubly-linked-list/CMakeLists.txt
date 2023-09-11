add_executable(doubly-linked-list-benchmark dll-benchmark.cpp)

target_link_libraries_system(doubly-linked-list-benchmark benchmark::benchmark)
target_link_libraries(doubly-linked-list-benchmark PUBLIC dcds)

target_compile_features(doubly-linked-list-benchmark PUBLIC cxx_std_20)

dcds_target_enable_default_warnings(doubly-linked-list-benchmark)

