add_subdirectory(
        data-structures
)

add_executable(counter-based-ds
#        counter-based-ds.cpp
#        counter-example.cpp
#        counter-benchmark.cpp
#        two-counter-example.cpp
#        linked-list.cpp
        lru_implementation.cpp
#        doubly_connected_node.cpp
#        composable_lru_implementation.cpp
#        1counter-benchmark.cpp
#        3counter-benchmark.cp
#        lru-benchmark.cpp
#        dll-benchmark.cpp
        )

target_link_libraries_system(counter-based-ds
        benchmark::benchmark
        absl::log
)
target_link_libraries(counter-based-ds
        PUBLIC
        LLVM::LLVM
        dcds
        counter-based-datastructures
        pthread)

target_compile_features(counter-based-ds PUBLIC cxx_std_20)

dcds_target_enable_default_warnings(counter-based-ds)

install(TARGETS counter-based-ds
        EXPORT counter-based-ds
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
)
