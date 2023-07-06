# Pure, regular C++ files
set(counter-based-datastructures_cxx
        lib/un-generated/counter.cpp
)

add_library(counter-based-datastructures SHARED
        ${counter-based-datastructures_cxx}
)

target_include_directories(counter-based-datastructures
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries_system(counter-based-datastructures
        absl::log
        cuckoo::cuckoo
        benchmark::benchmark
)

target_link_libraries(counter-based-datastructures
        PUBLIC
        vtune::vtune
        dcds
        ${CMAKE_DL_LIBS}
)

target_compile_features(counter-based-datastructures PUBLIC cxx_std_20)
