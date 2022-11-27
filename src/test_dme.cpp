#include <iostream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include "dme_loader.h"
#include "bone_map.h"

int main() {
    //assert(jenkins::oaat("somefilename") == 0xe9eb0404 && "OAAT returned incorrect value.");
    int i = 0;
    for(auto iter = BONE_HASHMAP.begin(); iter != BONE_HASHMAP.end(); iter++) {
        if(jenkins::oaat(iter->first) != iter->second) {
            std::cerr << i << " jenkins::oaat('" << iter->first << "') != 0x" << std::hex << std::setw(8) << std::setfill('0') << iter->second << std::endl;
            std::abort();
        }
        i++;
    }

    std::ifstream input("testfiles/Vehicle_TR_Mosquito_Base_Chassis_LOD0.dme", std::ios::binary | std::ios::ate);
    size_t length = input.tellg();
    input.seekg(0);
    uint8_t *data = new uint8_t[length];
    if(data == nullptr) {
        std::cerr << "Could not allocate buffer of length " << length << std::endl;
        return 1;
    }
    input.read((char*)data, length);

    DME mosquito(data, length);
    uint32_t magic = mosquito.magic();

    std::cout << std::hex << magic << std::endl;
    for(int i = 0; i < 4; i++) {
        std::cout << (char)(magic >> (8 * i));
    }
    std::cout << std::endl;

    std::cout << "Textures:" << std::endl;
    
    for(std::string texture : mosquito.dmat()->textures()) {
        std::cout << "    " << texture << std::endl;
    }

    for(uint32_t i = 0; i < mosquito.dmat()->material_count(); i++) {
        std::cout << "Namehash " << i << ": " << std::hex << mosquito.dmat()->material(i)->namehash() << std::dec << " " << mosquito.dmat()->material(i)->namehash() << std::endl;
    }
    
    std::cout << "All assertions passed" << std::endl;
    return 0;
}