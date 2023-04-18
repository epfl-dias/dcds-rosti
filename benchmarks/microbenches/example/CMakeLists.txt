
add_executable(example-bench
  example-main.cpp example-bench.cpp)

target_link_libraries_system(example-bench
  benchmark::benchmark
  absl::log
  )
target_link_libraries(example-bench
  PUBLIC
  util)

target_compile_features(example-bench PUBLIC cxx_std_20)

datapipes_target_enable_default_warnings(example-bench)

install(TARGETS example-bench
        EXPORT example-bench
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )