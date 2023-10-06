add_subdirectory(
        data-structures
)

add_executable(counter-based-ds
        1counter-benchmark.cpp
#        3counter-benchmark.cpp
        )

target_link_libraries_system(counter-based-ds benchmark::benchmark)
target_link_libraries(counter-based-ds PUBLIC dcds)

target_compile_features(counter-based-ds PUBLIC cxx_std_20)

dcds_target_enable_default_warnings(counter-based-ds)
