

add_subdirectory(data-structures)

add_executable(list-based-ds
        list-based-ds.cpp
        )

target_link_libraries_system(play benchmark::benchmark)

target_link_libraries(list-based-ds
        PUBLIC
        dcds
        list-based-datastructures)

target_compile_features(list-based-ds PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(list-based-ds)

install(TARGETS list-based-ds
        EXPORT list-based-ds
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )