#include "mrn.h"
#include <spdlog/spdlog.h>

using namespace warpgate::mrn;

constexpr uint32_t PACKET_ALIGNMENT = 16;

MRN::MRN(std::span<uint8_t> subspan, std::string name): buf_(subspan), m_name(name) {
    size_t offset = 0;
    spdlog::info("Loading MRN {}...", m_name);
    while(offset < buf_.size()) {
        std::shared_ptr<Packet> packet = std::make_shared<Packet>(buf_.subspan(offset));
        std::shared_ptr<SkeletonPacket> skeleton;
        std::shared_ptr<FilenamesPacket> filenames;
        std::vector<std::string> strings;
        switch(packet->header()->type()) {
        case PacketType::Skeleton:
            skeleton = std::make_shared<SkeletonPacket>(packet);
            spdlog::info("Found skeleton with name '{}'", skeleton->skeleton()->bone_names()->strings()[1]);
            packet = skeleton;
            break;
        case PacketType::FileNames:
            filenames = std::make_shared<FilenamesPacket>(packet);
            strings = filenames->files()->filenames()->strings();
            spdlog::info("NSA Files in this MRN:");
            for(auto it = strings.begin(); it != strings.end(); it++) {
                spdlog::info("    {}", *it);
            }
            packet = filenames;
            break;
        default:
            // Do nothing
            break;
        }
        offset += packet->size();
        if(offset % PACKET_ALIGNMENT != 0) {
            offset += PACKET_ALIGNMENT - offset % PACKET_ALIGNMENT;
        }
        m_packets.push_back(packet);
    }
    spdlog::info("Loaded {} packets from MRN {}", m_packets.size(), m_name);
}

std::string MRN::name() const {
    return m_name;
}

std::vector<std::shared_ptr<Packet>> MRN::packets() const {
    return m_packets;
}