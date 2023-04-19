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
target_link_libraries(mrn_loader PRIVATE spdlog::spdlog)

# add_custom_command(
#         TARGET dme_loader POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy
#                 ${CMAKE_CURRENT_SOURCE_DIR}/data/materials.json
#                 ${CMAKE_BINARY_DIR}/data/materials.json)
# if(MATERIALS_JSON_PORTABLE)
# set(MATERIALS_JSON_LOCATION_INTERNAL "data/materials.json" PARENT_SCOPE)
# else()
# set(MATERIALS_JSON_LOCATION_INTERNAL "${CMAKE_BINARY_DIR}/data/materials.json" PARENT_SCOPE)
# endif()