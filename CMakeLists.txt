cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

project(warpgate)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU"))
  add_compile_options("-Wno-deprecated")
endif()

set(MATERIALS_JSON_PORTABLE 0 CACHE BOOL "")

set(CMAKE_VERBOSE_MAKEFILE 1 CACHE BOOL "")

add_subdirectory(lib)

find_package(Vulkan REQUIRED)

add_definitions(-DMATERIALS_JSON_LOCATION="${MATERIALS_JSON_LOCATION}" -DMATERIALS_JSON_PORTABLE=${MATERIALS_JSON_PORTABLE})

add_executable(test_adr
  src/test_adr.cpp
  src/utils/adr.cpp
)

message("${BUILD_SHARED_LIBS}")
SET(PUGIXML_LINKED_LIBRARY pugixml::static)

target_include_directories(test_adr PUBLIC include/)
target_link_libraries(test_adr PRIVATE ${PUGIXML_LINKED_LIBRARY} spdlog::spdlog synthium::synthium)

add_executable(test_dme src/test_dme.cpp)
target_include_directories(test_dme PUBLIC include/ dme_loader/include/)
target_link_libraries(test_dme PRIVATE dme_loader spdlog::spdlog synthium::synthium)

add_executable(test_cnk
  src/test_cnk.cpp
)
target_link_libraries(test_cnk PRIVATE cnk_loader spdlog::spdlog)

add_executable(test_mrn
  src/test_mrn.cpp
)
target_include_directories(test_mrn PUBLIC include/)
target_link_libraries(test_mrn PUBLIC mrn_loader spdlog::spdlog)

add_executable(test_zone
  src/test_zone.cpp
)
target_include_directories(test_zone PUBLIC include/)
target_link_libraries(test_zone PRIVATE zone_loader spdlog::spdlog synthium::synthium)

add_executable(adr_converter 
    src/adr_converter.cpp
    src/utils/adr.cpp
    src/utils/gltf/common.cpp
    src/utils/gltf.cpp
    src/utils/materials_3.cpp 
    src/utils/sign.cpp 
    src/utils/textures.cpp
    src/utils/tsqueue.cpp 
)
target_include_directories(adr_converter PUBLIC 
  include/
  ${CMAKE_BINARY_DIR}/include/
  lib/internal/dme_loader/include/
  lib/external/argparse/include/
  lib/external/half/include/
  lib/external/tinygltf/
  lib/external/synthium/include)
target_link_libraries(adr_converter PRIVATE dme_loader ${PUGIXML_LINKED_LIBRARY} spdlog::spdlog tinygltf argparse synthium::synthium gli)

add_executable(decompress
  src/decompress.cpp
)
target_include_directories(decompress PUBLIC include/ PRIVATE lib/external/synthium/external/zlib ${CMAKE_BINARY_DIR}/lib/external/synthium/external/zlib)
target_link_libraries(decompress PRIVATE spdlog::spdlog argparse ZLIB::ZLIB)

add_executable(export
  src/export.cpp
)
target_include_directories(export PUBLIC include/ lib/internal/cnk_loader/include/)
target_link_libraries(export PRIVATE spdlog::spdlog synthium::synthium argparse Glob cnk_loader)

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
    Glob
    gli
    spdlog::spdlog 
    synthium::synthium 
    tinygltf 
)

add_executable(mrn_converter
    src/mrn_converter.cpp
)
target_include_directories(mrn_converter PUBLIC include/)
target_link_libraries(mrn_converter PRIVATE argparse Glob gli mrn_loader spdlog::spdlog synthium::synthium tinygltf)

add_executable(warpgate WIN32 
  src/warpgate.cpp
  src/utils/vulkan/model_renderer.cpp
  src/utils/hikogui/widgets/model_widget.cpp
)

target_include_directories(warpgate PUBLIC include/ lib/external/half/include/)
target_link_libraries(warpgate PRIVATE hikogui gli synthium::synthium dme_loader mrn_loader spdlog::spdlog)

#set_property(TARGET warpgate PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_custom_target(vulkan_warpgate_shaders)
add_shader_target(vulkan_warpgate_shaders
    src/utils/vulkan/shaders/character.vert
    src/utils/vulkan/shaders/character.frag
)


add_custom_target(vulkan_warpgate_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/lib/external/hikogui/resources ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/lib/external/hikogui/shaders ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/src/utils/vulkan/shaders ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/resources/shaders
    DEPENDS
    hikogui vulkan_warpgate_shaders
    resources/warpgate.png
)

add_dependencies(warpgate vulkan_warpgate_resources)


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
target_link_libraries(zone_converter PRIVATE cnk_loader dme_loader zone_loader ${PUGIXML_LINKED_LIBRARY} spdlog::spdlog tinygltf argparse synthium::synthium gli Glob)

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
add_dependencies(test_mrn version)
add_dependencies(test_zone version)