set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_USE_EXTERNAL_GOOGLETEST OFF CACHE BOOL "" FORCE)
set(ABSL_PROPAGATE_CXX_STD ON)
include(external/CMakeLists.txt.abseil.in)

# Make glog appear as system library to avoid header warnings
#get_target_property(glog_INCLUDE_DIR glog INTERFACE_INCLUDE_DIRECTORIES)
#target_include_directories(glog SYSTEM PUBLIC ${glog_INCLUDE_DIR})
#add_library(glog::glog ALIAS glog)

