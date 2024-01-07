# Pure, regular C++ files
set(bench-data-structures_cxx
        lib/dcds-generated/indexed-map.cpp
        lib/dcds-generated/list/doubly-linked-list.cpp
        lib/dcds-generated/list/doubly-linked-list-2.cpp
        lib/dcds-generated/list/fixed-size-doubly-linked-list.cpp
        lib/dcds-generated/list/lru-list.cpp
        )

add_library(bench-data-structures SHARED
        ${bench-data-structures_cxx}
        )

target_include_directories(bench-data-structures
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

#target_link_libraries_system(bench-data-structures
#        benchmark::benchmark
#        )

target_link_libraries(bench-data-structures
        PUBLIC
        dcds
        )

target_compile_features(bench-data-structures PUBLIC cxx_std_23)