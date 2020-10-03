cmake_minimum_required(VERSION 3.14)

project(po_tests LANGUAGES CXX)

option(TEST_INSTALLED_VERSION "Use the installed library for testing" OFF)
if(TEST_INSTALLED_VERSION)
  find_package(po REQUIRED CONFIG)
else()
  include(FetchContent)
  FetchContent_Declare(po SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
  FetchContent_MakeAvailable(po)
endif()

enable_testing()

function(add_test)
  cmake_parse_arguments(TEST "" "NAME" "ARGUMENTS" ${ARGN})

  add_executable(${TEST_NAME} source/${TEST_NAME}.cpp)
  target_include_directories(${TEST_NAME}
          PRIVATE
          $<BUILD_INTERFACE:${PROJECT_SOURC_DIR}/include>)
  target_link_libraries(${TEST_NAME} PRIVATE po::po)
  target_compile_features(${TEST_NAME} PRIVATE cxx_std_20)

  # Call original add_test
  _add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME} ${TEST_ARGUMENTS})
endfunction()

add_test(NAME example ARGUMENTS
        --pflag1-can0 --parg1-asdf=1.1 --parg1-qwer=2.2
        group1 --arg2=2
        group2 --arg3=3
        group3 --flag1 --marg1=file1.txt --marg1=file2.txt)
