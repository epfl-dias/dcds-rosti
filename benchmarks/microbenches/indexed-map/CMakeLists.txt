project(indexed-map VERSION 0.1 LANGUAGES CXX)


add_executable(indexed-map
        indexed-map-main.cpp
        )

target_link_libraries_system(play benchmark::benchmark)

target_link_libraries(indexed-map
        PUBLIC
        dcds
        bench-data-structures
)

target_compile_features(indexed-map PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(indexed-map)

install(TARGETS indexed-map
        EXPORT indexed-map
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )