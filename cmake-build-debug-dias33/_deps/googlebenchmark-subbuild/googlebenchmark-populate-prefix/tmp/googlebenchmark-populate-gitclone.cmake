# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitclone-lastrun.txt" AND EXISTS "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitinfo.txt" AND
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitclone-lastrun.txt" IS_NEWER_THAN "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git" 
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/google/benchmark" "googlebenchmark-src"
    WORKING_DIRECTORY "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/google/benchmark'")
endif()

execute_process(
  COMMAND "/usr/bin/git" 
          checkout "v1.7.1" --
  WORKING_DIRECTORY "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'v1.7.1'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitinfo.txt" "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/googlebenchmark-populate-gitclone-lastrun.txt'")
endif()
