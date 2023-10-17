# Pure, regular C++ files
set(dcds_test_cxx
        test-builder.cpp

        statements/conditional-statements.cpp


        )

add_executable(dcds_test
        ${dcds_test_cxx}
        )

target_include_directories(dcds_test
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_link_libraries_system(dcds_test
        absl::log
        cuckoo::cuckoo
        LLVM::LLVM
        GTest::gtest
        GTest::gtest_main
        )

target_link_libraries(dcds_test
        PUBLIC
        dcds)

target_compile_features(dcds_test PUBLIC cxx_std_20)

install(TARGETS dcds_test
        EXPORT dcds_test
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )
