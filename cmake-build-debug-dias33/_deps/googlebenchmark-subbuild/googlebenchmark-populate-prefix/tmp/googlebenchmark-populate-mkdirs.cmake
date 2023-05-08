# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-src"
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-build"
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix"
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/tmp"
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp"
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src"
  "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/googlebenchmark-subbuild/googlebenchmark-populate-prefix/src/googlebenchmark-populate-stamp/${subDir}")
endforeach()
