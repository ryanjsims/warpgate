#include <iostream>
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

int main() {
    logger::info("test_dme using dme_loader version {}", CPPDMOD_VERSION);
    //assert(jenkins::oaat("somefilename") == 0xe9eb0404 && "OAAT returned incorrect value.");
    int i = 0;
    for(auto iter = BONE_HASHMAP.begin(); iter != BONE_HASHMAP.end(); iter++) {
        if(jenkins::oaat(iter->first) != iter->second) {
            std::cerr << i << " jenkins::oaat('" << iter->first << "') != 0x" << std::hex << std::setw(8) << std::setfill('0') << iter->second << std::endl;
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

    std::vector<uint8_t> data_vector;
    data_vector = manager.get("Vehicle_TR_Mosquito_Base_Chassis_LOD0.dme").get_data();

    DME mosquito(std::span<uint8_t>(data_vector.data(), data_vector.size()));
    uint32_t magic = mosquito.magic();

    std::cout << std::hex << magic << std::endl;
    for(int i = 0; i < 4; i++) {
        std::cout << (char)(magic >> (8 * i));
    }
    std::cout << std::endl;

    std::cout << "Textures:" << std::endl;
    std::unordered_map<uint32_t, std::string> hashes_to_names;
    
    for(std::string texture : mosquito.dmat()->textures()) {
        std::string temp = utils::uppercase(texture);
        
        uint32_t namehash = jenkins::oaat(temp.c_str());
        hashes_to_names[namehash] = texture;
        std::cout << "    " << texture << std::hex << ": 0x" << std::setw(8) << std::setfill('0') << namehash << std::endl;
    }

    for(uint32_t i = 0; i < mosquito.dmat()->material_count(); i++) {
        std::cout << "Namehash " << i << ": " << std::hex << mosquito.dmat()->material(i)->namehash() << std::dec << " " << mosquito.dmat()->material(i)->namehash() << std::endl;
        for(uint32_t j = 0; j < mosquito.dmat()->material(i)->param_count(); j++) {
            if(mosquito.dmat()->material(i)->parameter(j).type() == Parameter::D3DXParamType::TEXTURE) {
                std::cout 
                        << std::dec 
                        << mosquito.dmat()->material(i)->parameter(j).semantic_hash() << ": " 
                        << hashes_to_names.at(mosquito.dmat()->material(i)->parameter(j).get<uint32_t>(16)) 
                        << std::endl;
            }
        }
    }
    
    std::cout << "All assertions passed" << std::endl;
    return 0;
}