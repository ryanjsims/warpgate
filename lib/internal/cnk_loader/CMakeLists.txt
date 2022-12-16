add_library(
        cnk_loader STATIC
        src/cnk0.cpp
        src/cnk1.cpp
        src/eco.cpp
        src/flora.cpp
        src/forgelight_chunk.cpp
        src/texture.cpp
        src/tile.cpp
        include/cnk_loader.h
)
target_include_directories(cnk_loader PUBLIC include)
target_link_libraries(cnk_loader PRIVATE spdlog::spdlog lzham)
