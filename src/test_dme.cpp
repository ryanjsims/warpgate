#include <fstream>
#include <iomanip>
#include <cassert>
#include <string>
#include <algorithm>
#include <filesystem>
#include "dme_loader.h"
#include "bone_map.h"
#include "utils.h"
#include "version.h"

#include <spdlog/spdlog.h>
#include <synthium/synthium.h>

namespace logger = spdlog;
using namespace warpgate;

int main() {
    logger::info("test_dme using dme_loader version {}", WARPGATE_VERSION);
    //assert(jenkins::oaat("somefilename") == 0xe9eb0404 && "OAAT returned incorrect value.");
    int i = 0;
    for(auto iter = BONE_HASHMAP.begin(); iter != BONE_HASHMAP.end(); iter++) {
        if(jenkins::oaat(iter->first) != iter->second) {
            logger::error("{} jenkins::oaat('{}') != 0x{:08x}", i, iter->first, iter->second);
            std::abort();
        }
        i++;
    }

    std::filesystem::path server = "C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/";
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 24; i++) {
        assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
    }
    logger::info("Loading packs...");
    synthium::Manager manager(assets);
    logger::info("Manager loaded.");

    std::vector<uint8_t> data_vector = manager.get("Vehicle_TR_Mosquito_Base_Chassis_LOD0.dme").get_data();

    DME mosquito(data_vector, "mosquito");
    uint32_t magic = mosquito.magic();

    logger::info("0x{:08x}", magic);
    for(int i = 0; i < 4; i++) {
        logger::info("{:4s}", (char*)&magic);
    }

    logger::info("Textures:");
    std::unordered_map<uint32_t, std::string> hashes_to_names;
    
    for(std::string texture : mosquito.dmat()->textures()) {
        std::string temp = utils::uppercase(texture);
        
        uint32_t namehash = jenkins::oaat(temp.c_str());
        hashes_to_names[namehash] = texture;
        logger::info("    {}: 0x{:08x}", texture, namehash);
    }

    for(uint32_t i = 0; i < mosquito.dmat()->material_count(); i++) {
        logger::info("Namehash {}: {:08x} {:d}", i, (uint32_t)mosquito.dmat()->material(i)->namehash(), (uint32_t)mosquito.dmat()->material(i)->namehash());
        for(uint32_t j = 0; j < mosquito.dmat()->material(i)->param_count(); j++) {
            if(mosquito.dmat()->material(i)->parameter(j).type() == Parameter::D3DXParamType::TEXTURE) {
                logger::info("{:d}: {}",
                    mosquito.dmat()->material(i)->parameter(j).semantic_hash(), 
                    hashes_to_names.at(mosquito.dmat()->material(i)->parameter(j).get<uint32_t>(16))
                ); 
            }
        }
    }
    
    logger::info("All assertions passed");
    return 0;
}