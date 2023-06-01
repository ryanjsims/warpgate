#pragma once

namespace warpgate::gtk {
    enum class AssetType {
        UNKNOWN,
        ACTOR_RUNTIME,
        ANIMATION,
        ANIMATION_NETWORK,
        ANIMATION_SET,
        MATERIAL,
        MODEL,
        PALETTE,
        PARAMETER,
        SKELETON,
        TEXTURE,
        VALUE,
        VIRTUAL,
    };
}