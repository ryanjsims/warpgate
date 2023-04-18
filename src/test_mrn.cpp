#include <fstream>
#include <iomanip>
#include <cassert>
#include <string>
#include <algorithm>
#include <filesystem>
#include "mrn_loader.h"
#include "version.h"
#include "utils/morpheme_loader.h"

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

    try {
        mrn::MRN aircraftX64(data_span, "AircraftX64.mrn");
    } catch(std::exception e) {
        logger::error("{}", e.what());
        return 1;
    }
    return 0;
}