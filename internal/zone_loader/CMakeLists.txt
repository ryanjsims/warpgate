add_library(
        zone_loader STATIC
        src/eco.cpp
        src/eco_layer.cpp
        src/flora.cpp
        src/flora_info.cpp
        src/instance.cpp
        src/light.cpp
        src/runtime_object.cpp
        src/texture_info.cpp
        src/zone.cpp
)
target_include_directories(zone_loader PUBLIC include)
target_link_libraries(zone_loader PRIVATE spdlog::spdlog)