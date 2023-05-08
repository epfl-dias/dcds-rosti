
cmake_minimum_required(VERSION 3.17.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")


#-----------------------------------------------------------------------
# Setup environment for the project
#-----------------------------------------------------------------------

# Whether to enable or not VTune support, if available
set(VTUNE_ENABLE TRUE)

# Force use Clang/LLVM/libcxx for now
set(LLVM_REQUIRED_VERSION 14)
find_program(CLANG_CXX_COMPILER "clang++${LLVM_VERSION_SUFFIX}" REQUIRED)
set(CMAKE_CXX_COMPILER "${CLANG_CXX_COMPILER}")
find_program(CLANG_C_COMPILER "clang${LLVM_VERSION_SUFFIX}" REQUIRED)
set(CMAKE_C_COMPILER "${CLANG_C_COMPILER}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")

#-----------------------------------------------------------------------
if (POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif (POLICY CMP0077)

#-----------------------------------------------------------------------
# Now that the compilers are set, define project name and version
project(dcds VERSION 0.1 LANGUAGES C CXX)

# This has to be done after enabling languages
include(GNUInstallDirs)
include(GetPrerequisites)

#-----------------------------------------------------------------------
# Setup link flags
#-----------------------------------------------------------------------
# Manage correctly Library path
# Per https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling#always-full-rpath

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if ("${isSystemDir}" STREQUAL "-1")
    # Make path relative to executable, so it works from build
    # or install directories, without having to set
    # LD_LIBRARY_PATH.
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
endif ("${isSystemDir}" STREQUAL "-1")

add_link_options(-ldl)


#-----------------------------------------------------------------------
# Setup Compilation flags
#-----------------------------------------------------------------------
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

set(CMAKE_CXX_STANDARD 20)
# Otherwise the -std=gnu++XX is used instead of the -std=c++XX, as one
# would expect from the property above
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CXX_STANDARD_REQUIRED TRUE)

#-----------------------------------------------------------------------
# Sanitize to comply with CMP0004
# For CMAKE variables, we can use string(STRIP "" var) for this

# Place each function and data in its own section
string(STRIP "${COMPILER_FLAGS} -ffunction-sections -fdata-sections" COMPILER_FLAGS)

# By default, tune for the local machine architecture
string(STRIP "${COMPILER_FLAGS} -march=native -mtune=native" COMPILER_FLAGS)

# Hide inline methods by default
string(STRIP "${COMPILER_FLAGS} -fvisibility-inlines-hidden" COMPILER_FLAGS)

string(STRIP "${COMPILER_FLAGS} -O3" COMPILER_FLAGS)

#-----------------------------------------------------------------------
# Add the compiler flags
string(STRIP "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS}" CMAKE_CXX_FLAGS)
string(STRIP "${CMAKE_C_FLAGS} ${COMPILER_FLAGS}" CMAKE_C_FLAGS)


#-----------------------------------------------------------------------
# Various dependencies
include(abseil)
include(gtest)
include(googlebenchmark)
include(vtune)
#include(memkind)
include(cuckoo)


option(${PROJECT_NAME}_ENABLE_CLANG_TIDY "Enable clang-tidy." ON)

#-----------------------------------------------------------------------
# Our cmake utilities
include(utils)
include(dcds-warning-flags)
include(doxygen)
include(clang-tidy)

#-----------------------------------------------------------------------
# Build our libraries
#-----------------------------------------------------------------------

add_subdirectory(benchmarks)
add_subdirectory(dcds)
