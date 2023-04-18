# Pure, regular C++ files
set(util_cxx
  lib/profiling.cpp
  )

add_library(util SHARED
  ${util_cxx}
  )

target_include_directories(util
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_link_libraries_system(util
  absl::log)
target_link_libraries(util
  PUBLIC
  vtune::vtune
  ${CMAKE_DL_LIBS}
  )

target_compile_features(util PUBLIC cxx_std_20)