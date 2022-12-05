cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(cppdmod)

if(NOT TARGET spdlog)
find_package(spdlog REQUIRED)
endif()

add_subdirectory(dme_loader)
add_subdirectory(external)
find_package(synthium REQUIRED)
find_package(gli REQUIRED)

add_definitions(-DMATERIALS_JSON_LOCATION="${MATERIALS_JSON_LOCATION}")

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC include/)
target_include_directories(test_dme PUBLIC dme_loader/include/)
target_link_libraries(test_dme PUBLIC dme_loader)
target_link_libraries(test_dme PRIVATE spdlog::spdlog)
target_link_libraries(test_dme PRIVATE synthium::synthium)


add_executable(dme_converter 
    src/dme_converter.cpp 
    src/utils/gltf.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
  )
target_include_directories(dme_converter PUBLIC include/)
target_include_directories(dme_converter PUBLIC ${CMAKE_BINARY_DIR}/include/)
target_include_directories(dme_converter PUBLIC dme_loader/include/)
target_include_directories(dme_converter PUBLIC external/argparse/include/)
target_include_directories(dme_converter PUBLIC external/half/include/)
target_include_directories(dme_converter PUBLIC external/tinygltf/)
target_link_libraries(dme_converter PRIVATE dme_loader)
target_link_libraries(dme_converter PRIVATE spdlog::spdlog)
target_link_libraries(dme_converter PRIVATE tinygltf)
target_link_libraries(dme_converter PRIVATE argparse)
target_link_libraries(dme_converter PRIVATE synthium::synthium)
target_link_libraries(dme_converter PRIVATE gli)

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/external/cmake-git-versioning/GenerateVersionHeader.cmake
  )
add_dependencies(dme_converter version)