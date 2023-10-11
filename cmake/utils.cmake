function(target_link_libraries_system target)
    # from https://stackoverflow.com/a/52136420/1237824
    set(libs ${ARGN})
    foreach(lib ${libs})
        get_target_property(lib_include_dirs ${lib} INTERFACE_INCLUDE_DIRECTORIES)
#        we may be linking against something without headers, e.g. benchmark::benchmark_main
        if(lib_include_dirs)
            target_include_directories(${target} SYSTEM PRIVATE ${lib_include_dirs})
        endif ()
        target_link_libraries(${target} PUBLIC ${lib})
    endforeach(lib)
endfunction(target_link_libraries_system)