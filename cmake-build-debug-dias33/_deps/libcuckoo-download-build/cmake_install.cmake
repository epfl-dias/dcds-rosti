# Install script for directory: /tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-src/libcuckoo

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/scratch/pelago/llvm-14/opt/bin/llvm-objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libcuckoo/libcuckoo-targets.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libcuckoo/libcuckoo-targets.cmake"
         "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-build/CMakeFiles/Export/share/cmake/libcuckoo/libcuckoo-targets.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libcuckoo/libcuckoo-targets-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/cmake/libcuckoo/libcuckoo-targets.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/libcuckoo" TYPE FILE FILES "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-build/CMakeFiles/Export/share/cmake/libcuckoo/libcuckoo-targets.cmake")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/cmake/libcuckoo" TYPE FILE FILES
    "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-src/libcuckoo/libcuckoo-config.cmake"
    "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-build/libcuckoo-config-version.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/include/libcuckoo/cuckoohash_config.hh;/usr/local/include/libcuckoo/cuckoohash_map.hh;/usr/local/include/libcuckoo/cuckoohash_util.hh;/usr/local/include/libcuckoo/bucket_container.hh")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/usr/local/include/libcuckoo" TYPE FILE FILES
    "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-src/libcuckoo/cuckoohash_config.hh"
    "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-src/libcuckoo/cuckoohash_map.hh"
    "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-src/libcuckoo/cuckoohash_util.hh"
    "/tmp/tmp.Y6NElX1M2E/cmake-build-debug-dias33/_deps/libcuckoo-download-src/libcuckoo/bucket_container.hh"
    )
endif()

