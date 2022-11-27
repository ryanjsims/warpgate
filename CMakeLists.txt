cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(cppdmod VERSION 0.0.1)

add_definitions(-DCPP_DMOD_VERSION="${CMAKE_PROJECT_VERSION}")

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
endif()

add_subdirectory(dme_loader)
add_subdirectory(external)

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC include/)
target_include_directories(test_dme PUBLIC dme_loader/include/)
target_link_libraries(test_dme PUBLIC dme_loader)
target_link_libraries(test_dme PRIVATE spdlog::spdlog)

add_executable(dme_converter src/dme_converter.cpp)
target_include_directories(dme_converter PUBLIC include/)
target_include_directories(dme_converter PUBLIC dme_loader/include/)
target_include_directories(dme_converter PUBLIC external/tinygltf/)
target_include_directories(dme_converter PUBLIC external/argparse/include/)
target_link_libraries(dme_converter PUBLIC dme_loader)
target_link_libraries(dme_converter PRIVATE spdlog::spdlog)
target_link_libraries(dme_converter PRIVATE tinygltf)
target_link_libraries(dme_converter PRIVATE argparse)