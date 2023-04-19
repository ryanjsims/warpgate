#include "mrn.h"
#include <spdlog/spdlog.h>

using namespace warpgate::mrn;

constexpr uint32_t PACKET_ALIGNMENT = 16;

MRN::MRN() {}

MRN::MRN(std::span<uint8_t> subspan, std::string name): buf_(subspan), m_name(name) {
    size_t offset = 0;
    spdlog::info("Loading MRN {}...", m_name);
    while(offset < buf_.size()) {
        std::shared_ptr<Packet> packet = std::make_shared<Packet>(buf_.subspan(offset));
        std::shared_ptr<SkeletonPacket> skeleton;
        std::shared_ptr<SkeletonNamesPacket> skeleton_names;
        std::shared_ptr<FilenamesPacket> filenames;
        std::shared_ptr<NSAFilePacket> nsa_file;
        std::vector<std::string> strings;
        switch(packet->header()->type()) {
        case PacketType::Skeleton:
            skeleton = std::make_shared<SkeletonPacket>(packet);
            spdlog::info("Found skeleton with name '{}'", skeleton->skeleton_data()->bone_names()->strings()[1]);
            packet = skeleton;
            break;
        case PacketType::FileNames:
            m_filenames_index = m_packets.size();
            filenames = std::make_shared<FilenamesPacket>(packet);
            strings = filenames->files()->filenames()->strings();
            spdlog::info("NSA Files in this MRN:");
            for(auto it = strings.begin(); it != strings.end(); it++) {
                spdlog::info("    {}", *it);
            }
            packet = filenames;
            break;
        case PacketType::SkeletonNames:
            m_skeletons_index = m_packets.size();
            skeleton_names = std::make_shared<SkeletonNamesPacket>(packet);
            spdlog::info("Skeletons in this MRN:");
            strings = skeleton_names->skeleton_names()->strings();
            for(auto it = strings.begin(); it != strings.end(); it++) {
                spdlog::info("    {}", *it);
            }
            packet = skeleton_names;
            break;
        case PacketType::NSAData:
            nsa_file = std::make_shared<NSAFilePacket>(packet);
            packet = nsa_file;
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

std::shared_ptr<SkeletonNamesPacket> MRN::skeleton_names() const {
    return std::static_pointer_cast<SkeletonNamesPacket>(m_packets[m_skeletons_index]);
}

std::shared_ptr<FilenamesPacket> MRN::file_names() const {
    return std::static_pointer_cast<FilenamesPacket>(m_packets[m_filenames_index]);
}
