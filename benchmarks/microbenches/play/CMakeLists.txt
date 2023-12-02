project(play VERSION 0.1 LANGUAGES CXX)


add_executable(play
        play-main.cpp
        )

target_link_libraries_system(play benchmark::benchmark)

target_link_libraries(play
        PUBLIC
        absl::check
        bench-data-structures
        dcds)

target_compile_features(play PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(play)

install(TARGETS play
        EXPORT play
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )