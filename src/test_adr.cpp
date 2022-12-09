#include "utils/adr.h"
#include "version.h"

#include <spdlog/spdlog.h>
#include <synthium/synthium.h>

namespace logger = spdlog;

int main() {
    logger::info("test_adr using warpgate version {}", WARPGATE_VERSION);
    std::filesystem::path server = "C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/";
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 24; i++) {
        assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
    }
    logger::info("Loading packs...");
    synthium::Manager manager(assets);
    logger::info("Manager loaded.");

    
    std::vector<uint8_t> data_vector = manager.get("Vehicle_TR_Mosquito_Base_Chassis.adr").get_data();
    
    logger::info("Got buffer:\n{}", (char*)data_vector.data());

    logger::info("Parsing ADR from buffer...");
    warpgate::utils::ADR mosquito(data_vector);
    logger::info("ADR parsed.");
    
    logger::info("Base DME name: {}", *mosquito.base_model());
    logger::info("Base DMAT name: {}", *mosquito.base_palette());
    logger::info("Anim MRN name: {}", *mosquito.animation_network());
    logger::info("Anim set name: {}", *mosquito.animation_set());

    return 0;
}