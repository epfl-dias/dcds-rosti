#project(lru-list-dcds VERSION 0.1 LANGUAGES CXX)


add_executable(lru-list-dcds
        # lru-dcds.cpp
        lru-dcds-main.cpp
        )

target_link_libraries_system(play benchmark::benchmark absl::check)

target_link_libraries(lru-list-dcds
        PUBLIC
        dcds
        bench-data-structures)

target_compile_features(lru-list-dcds PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(lru-list-dcds)

install(TARGETS lru-list-dcds
        EXPORT lru-list-dcds
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )