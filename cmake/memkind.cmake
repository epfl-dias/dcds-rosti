# careful with this, it is an in-source autogen + make build
# We copy the source into CMAKE_BINARY_DIR to do a debug and a release build
# but updates to this file probably require deleting cmake-build-*

include(external/CMakeLists.txt.cuckoo.in)

if (IS_ABSOLUTE CMAKE_INSTALL_PREFIX)
    set(DCDS_MEMKIND_INSTALL_PREFIX CMAKE_INSTALL_PREFIX)
else ()
    set(DCDS_MEMKIND_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_PREFIX}")
endif ()

# we copy the files into the CMAKE_BINARY_DIR, so we don't get conflicting
# builds when using multiple cmake profiles, e.g. Debug and Release
message(DEBUG "Copying memkind files to binary dir")
file(GLOB MEMKIND_SOURCE_FILES
        "${memkind-download_SOURCE_DIR}/*"
        )

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/external/memkind
        COMMAND_ERROR_IS_FATAL ANY)
file(COPY ${MEMKIND_SOURCE_FILES} DESTINATION ${CMAKE_BINARY_DIR}/external/memkind)

message(STATUS "memkind autogen in ${CMAKE_BINARY_DIR}/external/memkind")

set(ENV{CC} ${CMAKE_C_COMPILER})
set(ENV{CXX} ${CMAKE_CXX_COMPILER})
set(ENV{CFLAGS} "-march=native -mtune=native")
set(ENV{CXXFLAGS} "-march=native -mtune=native -stdlib=libc++")

#set(ENV{C_INCLUDE_PATH} ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
#if (CMAKE_BUILD_TYPE MATCHES Debug)
#    set(ENV{LDFLAGS} "-L${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/")
#else ()
#    set(ENV{LDFLAGS} "-L${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/")
#endif ()
execute_process(
        COMMAND ./autogen.sh
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external/memkind
        COMMAND_ERROR_IS_FATAL ANY
)
message(STATUS "memkind configure")

if (CMAKE_BUILD_TYPE MATCHES Debug)
    execute_process(
            COMMAND ./configure --prefix=${DCDS_MEMKIND_INSTALL_PREFIX} --enable-hwloc --enable-debug --enable-debug-jemalloc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external/memkind
            COMMAND_ERROR_IS_FATAL ANY
    )
else ()
    execute_process(
            COMMAND ./configure --prefix=${DCDS_MEMKIND_INSTALL_PREFIX} --enable-hwloc
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external/memkind
            COMMAND_ERROR_IS_FATAL ANY
    )
endif ()


message(STATUS "memkind build")
execute_process(
        COMMAND make
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external/memkind
        COMMAND_ERROR_IS_FATAL ANY
)

message(STATUS "memkind install")
execute_process(
        COMMAND make install
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/external/memkind
        COMMAND_ERROR_IS_FATAL ANY
)

include(CMakeFindDependencyMacro)

# Look for the header file.
find_path(MEMKIND_INCLUDE_DIR
        NAMES memkind.h
        HINTS ${DCDS_MEMKIND_INSTALL_PREFIX}/include
        DOC "memkind include directory")
mark_as_advanced(MEMKIND_INCLUDE_DIR)

# Look for the library.
find_library(MEMKIND_LIBRARY
        NAMES memkind libmemkind
        HINTS ${DCDS_MEMKIND_INSTALL_PREFIX}/lib
        DOC "memkind library")
mark_as_advanced(MEMKIND_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MEMKIND
        REQUIRED_VARS MEMKIND_LIBRARY MEMKIND_INCLUDE_DIR)

# Copy the results to the output variables and target.
if (MEMKIND_FOUND)
    set(MEMKIND_LIBRARIES "${MEMKIND_LIBRARY}")
    set(MEMKIND_INCLUDE_DIRS "${MEMKIND_INCLUDE_DIR}")

    if (NOT TARGET MEMKIND::MEMKIND)
        add_library(MEMKIND::MEMKIND UNKNOWN IMPORTED)
        set_target_properties(MEMKIND::MEMKIND PROPERTIES
                IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                IMPORTED_LOCATION "${MEMKIND_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MEMKIND_INCLUDE_DIRS}")
    endif ()
else ()
    MESSAGE(FATAL_ERROR "MEMKIND not found")
endif ()