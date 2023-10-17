# Pure, regular C++ files
set(dcds_cxx
        lib/dcds.cpp

        # Builder
        lib/builder/builder.cpp
        lib/builder/function-builder.cpp
        lib/builder/statement-builder.cpp
        lib/builder/expressions/expressions.cpp

        # Codegen
        lib/codegen/codegen.cpp
        lib/codegen/llvm-codegen/llvm-codegen.cpp
        lib/codegen/llvm-codegen/llvm-context.cpp
        lib/codegen/llvm-codegen/llvm-jit.cpp
        lib/codegen/llvm-codegen/functions.cpp
        lib/codegen/llvm-codegen/llvm-expression-visitor.cpp

        # Common
        lib/common/exceptions.cpp

        # Indexes
        # lib/indexes/art-node.cpp

        # Storage
        lib/storage/table-registry.cpp
        lib/storage/table.cpp

        # Transaction
        lib/transaction/transaction-manager.cpp

        # Util
        lib/util/profiling.cpp
)

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
        LLVM::LLVM
        ${CMAKE_DL_LIBS}
        )

target_compile_features(dcds PUBLIC cxx_std_20)

add_subdirectory(test)
