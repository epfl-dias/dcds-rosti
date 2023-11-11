project(counter VERSION 0.1 LANGUAGES CXX)


add_executable(counter
        counter-main.cpp
        )

target_link_libraries_system(counter
        benchmark::benchmark
        absl::log
        cuckoo::cuckoo
        tbbmalloc
        tbbmalloc_proxy
        )

target_link_libraries(counter
        PUBLIC
        LLVM::LLVM
        dcds)

target_compile_features(counter PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(counter)

install(TARGETS counter
        EXPORT counter
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )