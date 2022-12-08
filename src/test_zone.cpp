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
    warpgate::Zone nexus(data_vector);
    logger::info("Nexus.zone loaded.");

    logger::info("Magic: '{}'", 
        nexus.version() < 4 
            ? ((warpgate::ZoneHeader)nexus.header()).magic() 
            : ((warpgate::ZoneHeaderv45)nexus.header_v45()).magic()
    );

    logger::info("Version: {}", nexus.version());
    if(nexus.version() > 3) {
        logger::warn("Version 4/5 test not implemented");
        return 0;
    }

    warpgate::ZoneHeader header = nexus.header();

    logger::info("Offsets:");
    logger::info("   ecos:        {:#010x}", header.offsets.ecos);
    logger::info("   floras:      {:#010x}", header.offsets.floras);
    logger::info("   invis_walls: {:#010x}", header.offsets.invis_walls);
    logger::info("   objects:     {:#010x}", header.offsets.objects);
    logger::info("   lights:      {:#010x}", header.offsets.lights);
    logger::info("   unknown:     {:#010x}", header.offsets.unknown);

    return 0;
}