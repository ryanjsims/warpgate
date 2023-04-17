#pragma once
#include <stdint.h>

namespace warpgate::mrn {
    enum class PacketType : uint32_t {
        Skeleton = 0x01,
        SkeletonMap = 0x02,
        EventTrack = 0x03,
        EventTrackIK = 0x04,
        UnknownData = 0x07,
        NetworkData = 0x0A,
        PluginList = 0x0C,
        FileNames = 0x0E,
        SkeletonNames = 0x0F,
        NSAData = 0x10
    };
}