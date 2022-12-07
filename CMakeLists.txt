cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(warpgate)

add_subdirectory(external)
add_subdirectory(internal)

add_definitions(-DMATERIALS_JSON_LOCATION="${MATERIALS_JSON_LOCATION}")

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC include/)
target_include_directories(test_dme PUBLIC dme_loader/include/)
target_link_libraries(test_dme PUBLIC dme_loader)
target_link_libraries(test_dme PRIVATE spdlog::spdlog)
target_link_libraries(test_dme PRIVATE synthium::synthium)

add_executable(test_cnk
  src/test_cnk.cpp
)

target_link_libraries(test_cnk PRIVATE cnk_loader spdlog::spdlog)

add_executable(dme_converter 
    src/dme_converter.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
  )
target_include_directories(dme_converter PUBLIC include/ ${CMAKE_BINARY_DIR}/include/ internal/dme_loader/include/ external/argparse/include/ external/half/include/ external/tinygltf/ external/synthium/include)
target_link_libraries(dme_converter PRIVATE dme_loader spdlog::spdlog tinygltf argparse synthium::synthium gli)

add_executable(chunk_converter
    src/chunk_converter.cpp
    src/utils/gltf/chunk.cpp
    src/utils/gltf/common.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
  )

target_include_directories(chunk_converter 
  PUBLIC 
    include/ 
    ${CMAKE_BINARY_DIR}/include/ 
    internal/cnk_loader/include/
    external/argparse/include/ 
    external/half/include/ 
    external/synthium/include
    external/tinygltf/ 
)
target_link_libraries(chunk_converter 
  PRIVATE 
    argparse 
    cnk_loader 
    gli
    spdlog::spdlog 
    synthium::synthium 
    tinygltf 
)

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/external/cmake-git-versioning/GenerateVersionHeader.cmake
  )
add_dependencies(dme_converter version)
add_dependencies(chunk_converter version)