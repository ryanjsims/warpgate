#include <vector>

#include "zone_loader.h"
#include "version.h"

#include <spdlog/spdlog.h>
#include <synthium/synthium.h>

namespace logger = spdlog;

int main() {
    logger::info("test_zone using warpgate version {}", WARPGATE_VERSION);
    std::filesystem::path server = "C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/";
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 5; i++) {
        assets.push_back(server / ("Nexus_x64_" + std::to_string(i) + ".pack2"));
    }
    logger::info("Loading packs...");
    synthium::Manager manager(assets);
    logger::info("Manager loaded.");

    logger::info("Loading Nexus.zone...");
    std::vector<uint8_t> data_vector = manager.get("Nexus.zone").get_data();
    warpgate::zone::Zone nexus(data_vector);
    logger::info("Nexus.zone loaded.");

    logger::info("Magic: '{}'", 
        nexus.version() < 4 
            ? ((warpgate::zone::ZoneHeader)nexus.header()).magic() 
            : ((warpgate::zone::ZoneHeaderv45)nexus.header_v45()).magic()
    );

    logger::info("Version: {}", nexus.version());
    if(nexus.version() > 3) {
        logger::warn("Version 4/5 test not implemented");
        return 0;
    }

    warpgate::zone::ZoneHeader header = nexus.header();

    logger::info("Offsets:");
    logger::info("   ecos:        {:#010x}", header.offsets.ecos);
    logger::info("   floras:      {:#010x}", header.offsets.floras);
    logger::info("   invis_walls: {:#010x}", header.offsets.invis_walls);
    logger::info("   objects:     {:#010x}", header.offsets.objects);
    logger::info("   lights:      {:#010x}", header.offsets.lights);
    logger::info("   unknown:     {:#010x}", header.offsets.unknown);

    logger::info("Per Tile Info:");
    logger::info("    quad_count:   {:d}", header.per_tile.quad_count);
    logger::info("    width:        {:.2f}", header.per_tile.width);
    logger::info("    height:       {:.2f}", header.per_tile.height);
    logger::info("    vertex_count: {:d}", header.per_tile.vertex_count);

    logger::info("Chunk Info:");
    logger::info("    tile_count:  {:d}", header.chunk_info.tile_count);
    logger::info("    start_x:    {: d}", header.chunk_info.start_x);
    logger::info("    start_y:    {: d}", header.chunk_info.start_y);
    logger::info("    count_x:     {:d}", header.chunk_info.count_x);
    logger::info("    count_y:     {:d}", header.chunk_info.count_y);

    return 0;
}