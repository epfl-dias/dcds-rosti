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
include _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/compiler_depend.make

# Include the progress variables for this target.
include _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/progress.make

# Include the compile flags for this target's objects.
include _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/flags.make

_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o: _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/flags.make
_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o: _deps/abseil-download-src/absl/debugging/failure_signal_handler.cc
_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o: _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging && /scratch/pelago/llvm-14/opt/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o -MF CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o.d -o CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o -c /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-src/absl/debugging/failure_signal_handler.cc

_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.i"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging && /scratch/pelago/llvm-14/opt/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-src/absl/debugging/failure_signal_handler.cc > CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.i

_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.s"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging && /scratch/pelago/llvm-14/opt/bin/clang++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-src/absl/debugging/failure_signal_handler.cc -o CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.s

# Object files for target absl_failure_signal_handler
absl_failure_signal_handler_OBJECTS = \
"CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o"

# External object files for target absl_failure_signal_handler
absl_failure_signal_handler_EXTERNAL_OBJECTS =

_deps/abseil-download-build/absl/debugging/libabsl_failure_signal_handler.a: _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/failure_signal_handler.cc.o
_deps/abseil-download-build/absl/debugging/libabsl_failure_signal_handler.a: _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/build.make
_deps/abseil-download-build/absl/debugging/libabsl_failure_signal_handler.a: _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libabsl_failure_signal_handler.a"
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging && $(CMAKE_COMMAND) -P CMakeFiles/absl_failure_signal_handler.dir/cmake_clean_target.cmake
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/absl_failure_signal_handler.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/build: _deps/abseil-download-build/absl/debugging/libabsl_failure_signal_handler.a
.PHONY : _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/build

_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/clean:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging && $(CMAKE_COMMAND) -P CMakeFiles/absl_failure_signal_handler.dir/cmake_clean.cmake
.PHONY : _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/clean

_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/depend:
	cd /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /tmp/tmp.Y6NElX1M2E /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-src/absl/debugging /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33 /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : _deps/abseil-download-build/absl/debugging/CMakeFiles/absl_failure_signal_handler.dir/depend

