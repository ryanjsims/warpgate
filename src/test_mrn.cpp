#include <fstream>
#include <iomanip>
#include <cassert>
#include <string>
#include <algorithm>
#include <filesystem>
#include "mrn_loader.h"
#include "version.h"

#include <spdlog/spdlog.h>

namespace logger = spdlog;
using namespace warpgate;

int main() {
    logger::info("test_mrn using mrn_loader version {}", WARPGATE_VERSION);
    std::ifstream input("export/mrn/AircraftX64.mrn", std::ios::binary | std::ios::ate);
    if(input.fail()) {
        logger::error("Failed to open file '{}'", "export/mrn/AircraftX64.mrn");
        std::exit(2);
    }
    size_t length = input.tellg();
    input.seekg(0);
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(length);
    input.read((char*)data.get(), length);
    input.close();
    std::span<uint8_t> data_span = std::span<uint8_t>(data.get(), length);
    mrn::MRN aircraftX64;
    try {
        aircraftX64 = mrn::MRN(data_span, "AircraftX64.mrn");
    } catch(std::exception e) {
        logger::error("{}", e.what());
        return 1;
    }


    std::shared_ptr<mrn::FilenamesPacket> animation_names = aircraftX64.file_names();
    for(uint32_t i = 0; i < aircraftX64.packets().size(); i++) {
        std::shared_ptr<mrn::Packet> packet = aircraftX64.packets()[i];
        std::shared_ptr<mrn::NSAFilePacket> nsa_packet;
        std::shared_ptr<mrn::SkeletonPacket> skeleton_packet;
        switch(packet->header()->type()) {
        case mrn::PacketType::NSAData:
            nsa_packet = std::static_pointer_cast<mrn::NSAFilePacket>(packet);
            break;
        case mrn::PacketType::Skeleton:
            skeleton_packet = std::static_pointer_cast<mrn::SkeletonPacket>(packet);
            break;
        default:
            continue;
        }
        if(nsa_packet) {
            logger::info("Dequantizing animation {}...", animation_names->files()->animation_names()->strings()[i]);
            nsa_packet->animation()->dequantize();
        } else if(skeleton_packet) {
            skeleton_packet->skeleton_data()->skeleton()->log_recursive();
        }
    }
    return 0;
}