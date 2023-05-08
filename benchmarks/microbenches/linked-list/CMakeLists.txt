
add_executable(linked-list
        linked-list.cpp
        )

target_link_libraries_system(linked-list
        benchmark::benchmark
        absl::log
        )
target_link_libraries(linked-list
        PUBLIC
        dcds)

target_compile_features(linked-list PUBLIC cxx_std_20)

dcds_target_enable_default_warnings(linked-list)

install(TARGETS linked-list
        EXPORT linked-list
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )