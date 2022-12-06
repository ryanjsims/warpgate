add_library(
        dme_loader STATIC
        src/dme.cpp
        src/dmat.cpp
        src/jenkins.cpp
        src/material.cpp
        src/mesh.cpp
        src/parameter.cpp
        src/ps2_bone_map.cpp
        src/utils.cpp
        src/vertexstream.cpp
)
target_include_directories(dme_loader PUBLIC include)
target_link_libraries(dme_loader PRIVATE spdlog::spdlog)

add_custom_command(
        TARGET dme_loader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/data/materials.json
                ${CMAKE_BINARY_DIR}/data/materials.json)
set(MATERIALS_JSON_LOCATION_INTERNAL "${CMAKE_BINARY_DIR}/data/materials.json" PARENT_SCOPE)