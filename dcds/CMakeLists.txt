# Pure, regular C++ files
set(dcds_cxx
        lib/dcds.cpp

        # Builder
        lib/builder/builder.cpp
        lib/codegen/codegen.cpp
        lib/builder/storage.cpp

        # Indexes
        # lib/indexes/art-node.cpp

        # Storage
        lib/storage/table-registry.cpp
        lib/storage/table.cpp

        # Transaction
        lib/transaction/transaction-manager.cpp

        # Util
        lib/util/profiling.cpp
        include/dcds/context/DCDSContext.hpp lib/codegen/codegen.cpp include/dcds/builder/storage.hpp lib/builder/storage.cpp)

add_library(dcds SHARED
        ${dcds_cxx}
        )

target_include_directories(dcds
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_link_libraries_system(dcds
        absl::log
        cuckoo::cuckoo
        LLVM::LLVM)

if (VTUNE AND VTUNE_ENABLE)
    target_link_libraries(dcds PUBLIC vtune::vtune)
endif ()

target_link_libraries(dcds
        PUBLIC
        vtune::vtune
        ${CMAKE_DL_LIBS}
        )

target_compile_features(dcds PUBLIC cxx_std_20)