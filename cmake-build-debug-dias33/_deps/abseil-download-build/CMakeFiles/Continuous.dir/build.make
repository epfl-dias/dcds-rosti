# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.23

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Produce verbose output by default.
VERBOSE = 1

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /scratch/pelago/builds/llvm-14/opt/bin/cmake

# The command to remove a file.
RM = /scratch/pelago/builds/llvm-14/opt/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /tmp/tmp.Y6NElX1M2E

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33

# Utility rule file for Continuous.

# Include any custom commands dependencies for this target.
include _deps/abseil-download-build/CMakeFiles/Continuous.dir/compiler_depend.make

# Include the progress variables for this target.
include _deps/abseil-download-build/CMakeFiles/Continuous.dir/progress.make

_deps/abseil-download-build/CMakeFiles/Continuous:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build && /scratch/pelago/builds/llvm-14/opt/bin/ctest -D Continuous

Continuous: _deps/abseil-download-build/CMakeFiles/Continuous
Continuous: _deps/abseil-download-build/CMakeFiles/Continuous.dir/build.make
.PHONY : Continuous

# Rule to build all files generated by this target.
_deps/abseil-download-build/CMakeFiles/Continuous.dir/build: Continuous
.PHONY : _deps/abseil-download-build/CMakeFiles/Continuous.dir/build

_deps/abseil-download-build/CMakeFiles/Continuous.dir/clean:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build && $(CMAKE_COMMAND) -P CMakeFiles/Continuous.dir/cmake_clean.cmake
.PHONY : _deps/abseil-download-build/CMakeFiles/Continuous.dir/clean

_deps/abseil-download-build/CMakeFiles/Continuous.dir/depend:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /tmp/tmp.Y6NElX1M2E /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-src /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33 /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/CMakeFiles/Continuous.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : _deps/abseil-download-build/CMakeFiles/Continuous.dir/depend

