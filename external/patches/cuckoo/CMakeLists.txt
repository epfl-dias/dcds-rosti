project(cuckoo VERSION 0.1 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)
target_compile_features (cuckoo INTERFACE cxx_std_17)

target_include_directories(${PROJECT_NAME} SYSTEM
        INTERFACE
        $<BUILD_INTERFACE:${libcuckoo-download_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )

#set(_proteus_install_target ${PROJECT_NAME})

#include(_proteus-install-lib)

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
#install(EXPORT      ${PROJECT_NAME}Targets
#        FILE        ${PROJECT_NAME}Targets.cmake
#        NAMESPACE   ${PROJECT_NAME}::
#        DESTINATION lib/cmake/${PROJECT_NAME}
##        COMPONENT   ${_proteus_install_dev}
#        )

export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_NAME}Config.cmake)

add_library(cuckoo::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
