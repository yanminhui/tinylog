cmake_minimum_required(VERSION 3.1.3)
include(GNUInstallDirs)

project(tinylog VERSION 1.4.0)


# Option
set(BUILD_SHARED_LIBS ON)


# Build
add_library(tinylog src/tinylog.cpp)
set_target_properties(tinylog PROPERTIES
  VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")

target_compile_features(tinylog PUBLIC cxx_extern_templates) # cxx_std_11
target_include_directories(tinylog PUBLIC include)


# Install
install(TARGETS tinylog LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/tinylog
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
