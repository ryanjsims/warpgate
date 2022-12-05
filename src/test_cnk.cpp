#include "forgelight_chunk.h"
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

    Chunk chunk(data_span);

    std::unique_ptr<uint8_t[]> decompressed = chunk.decompress();
    logger::info("Decompressed successfully!");
    logger::info("{}", logger::to_hex((char*)(decompressed.get()), (char*)(decompressed.get() + 512)));

    return 0;
}