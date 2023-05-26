#pragma once

namespace warpgate::gtk {
    enum class AssetType {
        UNKNOWN,
        ACTOR_RUNTIME,
        ANIMATION,
        ANIMATION_NETWORK,
        ANIMATION_SET,
        MODEL,
        PALETTE,
        SKELETON,
        TEXTURE,
        VIRTUAL
    };
}