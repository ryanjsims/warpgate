add_library(
        cnk_loader STATIC
        src/forgelight_chunk.cpp
)
target_include_directories(cnk_loader PUBLIC include)
target_link_libraries(cnk_loader PRIVATE spdlog::spdlog lzham)
