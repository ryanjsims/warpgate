add_library(
        mrn_loader STATIC
        src/mrn.cpp
        src/file_data.cpp
        src/nsa_file.cpp
        src/packet.cpp
        src/skeleton_data.cpp
        src/string_table.cpp
        src/utils.cpp
)
target_include_directories(mrn_loader PUBLIC include)
target_link_libraries(mrn_loader PRIVATE spdlog::spdlog PUBLIC gli)
