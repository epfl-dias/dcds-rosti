project(play VERSION 0.1 LANGUAGES CXX)


add_executable(play
        play-main.cpp
        )

target_link_libraries_system(play
        benchmark::benchmark
        absl::log
        cuckoo::cuckoo
        )

target_link_libraries(play
        PUBLIC
        LLVM::LLVM
        dcds
        counter-based-datastructures)

target_compile_features(play PUBLIC cxx_std_20)

dcds_target_enable_default_warnings(play)

install(TARGETS play
        EXPORT play
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )