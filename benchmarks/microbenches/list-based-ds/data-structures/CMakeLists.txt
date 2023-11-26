# Pure, regular C++ files
set(list-based-datastructures_cxx
        lib/parser-generated/doubly-linked-list.cpp
        lib/parser-generated/linked-list.cpp
        lib/un-generated/linked-list.cpp
        )

add_library(list-based-datastructures SHARED
        ${list-based-datastructures_cxx}
        )

target_include_directories(list-based-datastructures
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_link_libraries_system(list-based-datastructures
        benchmark::benchmark
        )

target_link_libraries(list-based-datastructures
        PUBLIC
        dcds
        )

target_compile_features(list-based-datastructures PUBLIC cxx_std_23)