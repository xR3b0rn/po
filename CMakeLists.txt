cmake_minimum_required(VERSION 3.14)

project(po VERSION 0.0.1 LANGUAGES CXX)

# ---- Warning guard ----

# Protect dependents from this project's warnings if the guard isn't disabled
set(po_warning_guard "SYSTEM")
if(po_INCLUDE_WITHOUT_SYSTEM)
  set(po_warning_guard "")
endif()

# ---- Declare library ----

add_library(po INTERFACE)
add_library(po::po ALIAS po)

target_include_directories(po
        ${po_warning_guard}
        INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_compile_features(po INTERFACE cxx_std_17)

# ---- Install ----

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

write_basic_package_version_file(
        poConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
        ARCH_INDEPENDENT)

set(po_directory "po-${PROJECT_VERSION}")
set(po_include_directory "${CMAKE_INSTALL_INCLUDEDIR}/${po_directory}")

install(DIRECTORY "${PROJECT_SOURCE_DIR}/include/" DESTINATION "${po_include_directory}")

install(TARGETS po EXPORT poTargets INCLUDES DESTINATION "${po_include_directory}")

set(po_install_cmakedir "${CMAKE_INSTALL_LIBDIR}/cmake/${po_directory}")

install(EXPORT poTargets NAMESPACE po:: DESTINATION "${po_install_cmakedir}")

install(FILES
        "${PROJECT_SOURCE_DIR}/cmake/poConfig.cmake"
        "${PROJECT_BINARY_DIR}/poConfigVersion.cmake"
        DESTINATION "${po_install_cmakedir}")
