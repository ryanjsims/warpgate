add_library(dme_loader SHARED src/data_classes.cpp src/jenkins.cpp)
target_include_directories(dme_loader PUBLIC include)
target_link_libraries(dme_loader PRIVATE spdlog::spdlog)

add_custom_command(
        TARGET dme_loader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/data/materials.json
                ${CMAKE_BINARY_DIR}/data/materials.json)
set(MATERIALS_JSON_LOCATION "${CMAKE_BINARY_DIR}/data/materials.json" PARENT_SCOPE)