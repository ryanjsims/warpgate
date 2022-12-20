#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>

#include "version.h"

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>
#include <zlib.h>

namespace logger = spdlog;

uint32_t ntohl(uint32_t const net) {
    uint8_t data[4] = {};
    memcpy(&data, &net, sizeof(data));

    return ((uint32_t) data[3] << 0)
         | ((uint32_t) data[2] << 8)
         | ((uint32_t) data[1] << 16)
         | ((uint32_t) data[0] << 24);
}

void build_argument_parser(argparse::ArgumentParser &parser, int &log_level) {
    parser.add_description("Forgelight Asset Decompressor");
    parser.add_argument("compressed_file");
    parser.add_argument("output_file");
}

int main(int argc, char* argv[]) {
    logger::info("Decompressing '{}' (using decompress {})", WARPGATE_VERSION);
    argparse::ArgumentParser parser("decompress", WARPGATE_VERSION);
    int log_level = logger::level::info;
    build_argument_parser(parser, log_level);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
    logger::set_level(logger::level::level_enum(log_level));

    std::string input_filename = parser.get<std::string>("compressed_file");

    std::ifstream input(input_filename, std::ios::binary | std::ios::ate);
    if(input.fail()) {
        logger::error("Failed to open '{}': {}", input_filename, strerror(errno));
        std::exit(1);
    }
    size_t length = input.tellg();
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(length);
    input.seekg(0);
    input.read((char*)data.get(), length);

    std::span<uint32_t> header(reinterpret_cast<uint32_t*>(data.get()), 2);
    if(ntohl(header[0]) != 0xA1B2C3D4) {
        logger::error("Invalid header, not a compressed forgelight asset. Got magic {:#010x}", ntohl(header[0]));
        std::exit(2);
    }

    unsigned long compressed_size = (unsigned long)(length - 8);
    unsigned long decompressed_size = ntohl(header[1]);
    logger::info("Decompressing asset '{}' of length {} (compressed length {})", input_filename, decompressed_size, compressed_size);

    std::unique_ptr<uint8_t[]> decompressed = std::make_unique<uint8_t[]>(decompressed_size);
    int errcode = uncompress2(decompressed.get(), &decompressed_size, data.get() + 8, &compressed_size);

    if(errcode != Z_OK) {
        switch(errcode) {
        case Z_MEM_ERROR:
            logger::error("uncompress: Not enough memory! ({})", errcode);
            break;
        case Z_BUF_ERROR:
            // Theoretically this shouldn't happen
            logger::error("uncompress: Not enough room in the output buffer! ({})", errcode);
            break;
        case Z_DATA_ERROR:
            logger::error("uncompress: Input data was corrupted or incomplete! ({})", errcode);
            break;
        }
        logger::error("Error message: {}", zError(errcode));
        std::exit(4);
    }
    
    std::filesystem::path output_filename(parser.get<std::string>("output_file"));
    output_filename = std::filesystem::weakly_canonical(output_filename);
    std::filesystem::path output_directory;
    if(output_filename.has_parent_path()) {
        output_directory = output_filename.parent_path();
    }

    try {
        if(!std::filesystem::exists(output_filename.parent_path())) {
            std::filesystem::create_directories(output_directory);
        }
    } catch (std::filesystem::filesystem_error& err) {
        logger::error("Failed to create directory {}: {}", err.path1().string(), err.what());
        std::exit(3);
    }

    logger::info("Writing file {}", output_filename.string());
    std::ofstream output(output_filename, std::ios::binary);
    output.write((char*)decompressed.get(), decompressed_size);
    logger::info("Wrote {} bytes to {}", decompressed_size, output_filename.string());
    return 0;
}