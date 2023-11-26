project(TPCC VERSION 0.1 LANGUAGES CXX)


add_executable(TPCC
        TPCC.cpp
        TPCC-main.cpp
        )

target_link_libraries_system(play benchmark::benchmark)

target_link_libraries(TPCC
        PUBLIC
        dcds)

target_compile_features(TPCC PUBLIC cxx_std_23)

dcds_target_enable_default_warnings(TPCC)

install(TARGETS TPCC
        EXPORT TPCC
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )