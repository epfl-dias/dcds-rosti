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

# Include any dependencies generated for this target.
include _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/compiler_depend.make

# Include the progress variables for this target.
include _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/progress.make

# Include the compile flags for this target's objects.
include _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/flags.make

_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.o: _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/flags.make
_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.o: _deps/googlebenchmark-src/src/benchmark_main.cc
_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.o: _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.o"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src && /scratch/pelago/llvm-14/opt/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.o -MF CMakeFiles/benchmark_main.dir/benchmark_main.cc.o.d -o CMakeFiles/benchmark_main.dir/benchmark_main.cc.o -c /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src/src/benchmark_main.cc

_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/benchmark_main.dir/benchmark_main.cc.i"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src && /scratch/pelago/llvm-14/opt/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src/src/benchmark_main.cc > CMakeFiles/benchmark_main.dir/benchmark_main.cc.i

_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/benchmark_main.dir/benchmark_main.cc.s"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src && /scratch/pelago/llvm-14/opt/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src/src/benchmark_main.cc -o CMakeFiles/benchmark_main.dir/benchmark_main.cc.s

# Object files for target benchmark_main
benchmark_main_OBJECTS = \
"CMakeFiles/benchmark_main.dir/benchmark_main.cc.o"

# External object files for target benchmark_main
benchmark_main_EXTERNAL_OBJECTS =

_deps/googlebenchmark-build/src/libbenchmark_main.a: _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/benchmark_main.cc.o
_deps/googlebenchmark-build/src/libbenchmark_main.a: _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/build.make
_deps/googlebenchmark-build/src/libbenchmark_main.a: _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libbenchmark_main.a"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src && $(CMAKE_COMMAND) -P CMakeFiles/benchmark_main.dir/cmake_clean_target.cmake
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/benchmark_main.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/build: _deps/googlebenchmark-build/src/libbenchmark_main.a
.PHONY : _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/build

_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/clean:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src && $(CMAKE_COMMAND) -P CMakeFiles/benchmark_main.dir/cmake_clean.cmake
.PHONY : _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/clean

_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/depend:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /tmp/tmp.Y6NElX1M2E /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src/src /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33 /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : _deps/googlebenchmark-build/src/CMakeFiles/benchmark_main.dir/depend

