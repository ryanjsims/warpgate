add_library(
        dme_loader STATIC
        src/dme.cpp
        src/dmat.cpp
        src/jenkins.cpp
        src/material.cpp
        src/mesh.cpp
        src/parameter.cpp
        src/ps2_bone_map.cpp
        src/semantics.cpp
        src/utils.cpp
        src/vertexstream.cpp
)
target_include_directories(dme_loader PUBLIC include)
target_link_libraries(dme_loader PRIVATE spdlog::spdlog)
