# cache is neccessary because tbb uses cmake minimumun version 3.1 and does not set CMP0077
SET(TBB_TEST OFF CACHE BOOL "build tbb tests")
SET(TBB_STRICT OFF CACHE BOOL "warnings as errors for tbb")
include(external/CMakeLists.txt.tbb.in)