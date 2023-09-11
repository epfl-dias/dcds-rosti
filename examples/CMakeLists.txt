file(GLOB_RECURSE _examples ${CMAKE_CURRENT_LIST_DIR}/*.cpp CONFIGURE_DEPEND)

foreach(_example ${_examples})
    get_filename_component(_name ${_example} NAME_WE)
    add_executable(example_${_name} ${_name}.cpp)

    target_link_libraries_system(example_${_name}
            benchmark::benchmark
            absl::log
            )
    target_link_libraries(example_${_name}
            PUBLIC
            LLVM::LLVM
            dcds
            pthread)

    target_compile_features(example_${_name} PUBLIC cxx_std_20)

    dcds_target_enable_default_warnings(example_${_name})

    install(TARGETS example_${_name}
            EXPORT example_${_name}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
            INCLUDES DESTINATION include)
endforeach()

unset(_example)
unset(_examples)
