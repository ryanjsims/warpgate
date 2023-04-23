#pragma once

#include "dme.h"

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <memory>
#include <vector>

namespace warpgate::vulkan {
    struct Mesh {
        std::vector<VkBuffer> vertexStreams;
        std::vector<VmaAllocation> vertexStreamAllocations;
        VkBuffer indices;
        VmaAllocation indicesAllocation;
        uint32_t index_count, index_size;
    };

    struct Model {
        std::shared_ptr<DME> data;
        std::vector<Mesh> meshes;
    };
}