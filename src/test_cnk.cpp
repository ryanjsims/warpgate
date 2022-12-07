#include "cnk_loader.h"
#include <fstream>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

namespace logger = spdlog;

int main() {
    logger::info("Loading chunk...");
    std::ifstream input("export/cnk/Nexus_0_0.cnk0", std::ios::binary | std::ios::ate);
    if(input.fail()) {
        logger::error("Failed to open file '{}'", "export/cnk/Nexus_0_0.cnk0");
        std::exit(2);
    }
    size_t length = input.tellg();
    input.seekg(0);
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(length);
    input.read((char*)data.get(), length);
    input.close();
    std::span<uint8_t> data_span = std::span<uint8_t>(data.get(), length);

    warpgate::Chunk chunk(data_span);

    std::unique_ptr<uint8_t[]> decompressed = chunk.decompress();
    logger::info("Decompressed successfully!");
    logger::info("{}", logger::to_hex((char*)(decompressed.get()), (char*)(decompressed.get() + 512)));
    
    warpgate::ChunkHeader header = chunk.header();
    if(std::strncmp(header.magic, "CNK0", 4) == 0) {
        warpgate::CNK0 chunk0(std::span<uint8_t>(decompressed.get(), sizeof(warpgate::ChunkHeader) + chunk.decompressed_size()));
        logger::info("Chunk has {} indices and {} vertices", chunk0.index_count(), chunk0.vertex_count());

        std::span<warpgate::Vertex> vertices = chunk0.vertices();
        std::span<warpgate::RenderBatch> render_batches = chunk0.render_batches();
        for(uint32_t batch = 0; batch < render_batches.size(); batch++){
            auto[minimum, maximum] = chunk0.aabb(batch);
            logger::info("AABB of render batch {}: ({}, {}, {}) to ({}, {}, {})", batch, minimum.x, minimum.y, (float)minimum.height_far / 32.0f, maximum.x, maximum.y, (float)maximum.height_far / 32.0f);
        }
    }
    
    std::ofstream output("export/cnk/Nexus_0_0_dec.cnk0", std::ios::binary);
    if(output.fail()) {
        logger::error("Failed to open file '{}' for writing", "export/cnk/Nexus_0_0_dec.cnk0");
        std::exit(2);
    }
    output.write((char*)decompressed.get(), chunk.decompressed_size() + sizeof(warpgate::ChunkHeader));
    output.close();
    return 0;
}