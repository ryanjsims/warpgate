#include <iostream>
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

    std::cout << "All assertions passed" << std::endl;
    return 0;
}