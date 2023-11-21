project(YCSB VERSION 0.1 LANGUAGES CXX)


add_executable(YCSB
        YCSB-main.cpp
        )

target_link_libraries_system(YCSB
        benchmark::benchmark
        absl::log
        cuckoo::cuckoo
        tbbmalloc
        tbbmalloc_proxy
        )

target_link_libraries(YCSB
        PUBLIC
        LLVM::LLVM
        dcds)

target_compile_features(YCSB PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(YCSB)

install(TARGETS YCSB
        EXPORT YCSB
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )