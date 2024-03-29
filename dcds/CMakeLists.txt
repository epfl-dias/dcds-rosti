# Pure, regular C++ files
set(dcds_cxx
        lib/dcds.cpp

        # Builder
        lib/builder/builder.cpp
        lib/builder/function-builder.cpp
        lib/builder/statement.cpp
        lib/builder/statement-builder.cpp
        lib/builder/expressions/expressions.cpp
        lib/builder/builder-opt-passes.cpp
        lib/builder/cc-injector.cpp

        # Codegen
        lib/codegen/codegen.cpp
        lib/codegen/llvm-codegen/llvm-codegen.cpp
        lib/codegen/llvm-codegen/llvm-codegen-statement.cpp
        lib/codegen/llvm-codegen/llvm-codegen-function.cpp
        lib/codegen/llvm-codegen/llvm-context.cpp
        lib/codegen/llvm-codegen/llvm-jit.cpp
        lib/codegen/llvm-codegen/functions.cpp
        lib/codegen/llvm-codegen/llvm-expression-visitor.cpp
        lib/codegen/llvm-codegen/llvm-utils-conditionals.cpp
        lib/codegen/llvm-codegen/llvm-utils-loops.cpp

        # Common
        lib/common/exceptions.cpp
        lib/common/types.cpp

        # Indexes
        lib/indexes/index-functions.cpp

        # Storage
        lib/storage/table-registry.cpp
        lib/storage/table.cpp

        # Transaction
        lib/transaction/transaction-manager.cpp
        lib/transaction/transaction.cpp
        lib/transaction/txn-log.cpp

        # Util
        lib/util/profiling.cpp
        lib/util/logging.cpp
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
        absl::check
        absl::log_initialize
        absl::debugging
        absl::failure_signal_handler
        cuckoo::cuckoo
        tbb
        tbbmalloc
        tbbmalloc_proxy
        LLVM::LLVM)

if (VTUNE AND VTUNE_ENABLE)
    target_link_libraries(dcds PUBLIC vtune::vtune)
endif ()

target_link_libraries(dcds
        PUBLIC
        ${CMAKE_DL_LIBS}
        )

target_compile_features(dcds PUBLIC cxx_std_20)

add_subdirectory(test)
