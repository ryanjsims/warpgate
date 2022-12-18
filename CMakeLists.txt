cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(warpgate)

add_subdirectory(lib)

add_definitions(-DMATERIALS_JSON_LOCATION="${MATERIALS_JSON_LOCATION}")

add_executable(test_adr
  src/test_adr.cpp
  src/utils/adr.cpp
)
target_include_directories(test_adr PUBLIC include/)
target_link_libraries(test_adr PRIVATE pugixml::static spdlog::spdlog synthium::synthium)

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC include/ dme_loader/include/)
target_link_libraries(test_dme PRIVATE dme_loader spdlog::spdlog synthium::synthium)

add_executable(test_cnk
  src/test_cnk.cpp
)
target_link_libraries(test_cnk PRIVATE cnk_loader spdlog::spdlog)

add_executable(test_zone
  src/test_zone.cpp
)
target_include_directories(test_zone PUBLIC include/)
target_link_libraries(test_zone PRIVATE zone_loader spdlog::spdlog synthium::synthium)

add_executable(export
  src/export.cpp
)
target_include_directories(export PUBLIC include/)
target_link_libraries(export PRIVATE spdlog::spdlog synthium::synthium argparse Glob)

add_executable(dme_converter 
    src/dme_converter.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
)
target_include_directories(dme_converter PUBLIC 
  include/
  ${CMAKE_BINARY_DIR}/include/
  lib/internal/dme_loader/include/
  lib/external/argparse/include/
  lib/external/half/include/
  lib/external/tinygltf/
  lib/external/synthium/include)
target_link_libraries(dme_converter PRIVATE dme_loader spdlog::spdlog tinygltf argparse synthium::synthium gli)

add_executable(chunk_converter
    src/chunk_converter.cpp
    src/utils/gltf/chunk.cpp
    src/utils/gltf/common.cpp
    src/utils/aabb.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp
)

target_include_directories(chunk_converter 
  PUBLIC 
    include/ 
    ${CMAKE_BINARY_DIR}/include/ 
    lib/internal/cnk_loader/include/
    lib/internal/dme_loader/include/
    lib/external/argparse/include/ 
    lib/external/half/include/ 
    lib/external/synthium/include
    lib/external/tinygltf/ 
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

add_executable(zone_converter 
    src/zone_converter.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf/chunk.cpp
    src/utils/aabb.cpp
    src/utils/adr.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
  )
target_include_directories(zone_converter PUBLIC 
  include/
  ${CMAKE_BINARY_DIR}/include/
  lib/internal/cnk_loader/include/
  lib/internal/dme_loader/include/
  lib/external/argparse/include/
  lib/external/half/include/
  lib/external/tinygltf/
  lib/external/synthium/include
)
target_link_libraries(zone_converter PRIVATE cnk_loader dme_loader zone_loader pugixml::static spdlog::spdlog tinygltf argparse synthium::synthium gli Glob)

find_package(Git)
add_custom_target(version
  ${CMAKE_COMMAND} -D SRC=${CMAKE_SOURCE_DIR}/include/version.h.in
                   -D DST=${CMAKE_SOURCE_DIR}/include/version.h
                   -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
                   -P ${CMAKE_SOURCE_DIR}/lib/external/cmake-git-versioning/GenerateVersionHeader.cmake
)

add_dependencies(chunk_converter version)
add_dependencies(export version)
add_dependencies(dme_converter version)
add_dependencies(zone_converter version)

add_dependencies(test_cnk version)
add_dependencies(test_dme version)
add_dependencies(test_zone version)