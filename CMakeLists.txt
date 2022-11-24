cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(cppdmod)

add_subdirectory(dme_loader)

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC dme_loader/include/)
target_include_directories(test_dme PUBLIC include/)
target_link_libraries(test_dme dme_loader)
