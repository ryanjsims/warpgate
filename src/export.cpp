#include <fstream>
#include <sstream>

#include "version.h"

#include <argparse/argparse.hpp>
#include <cnk_loader.h>
#include <glob/glob.h>
#include <synthium/synthium.h>

namespace logger = spdlog;

void build_argument_parser(argparse::ArgumentParser &parser, int &log_level) {
    parser.add_description("Pack2 Asset Exporter");
    parser.add_argument("asset_name");
    parser.add_argument("output_file");
    
    parser.add_argument("--verbose", "-v")
        .help("Increase log level. May be specified multiple times")
        .action([&](const auto &){ 
            if(log_level > 0) {
                log_level--; 
            }
        })
        .append()
        .nargs(0)
        .default_value(false)
        .implicit_value(true);
    
    parser.add_argument("--assets-directory", "-d")
        .help("The directory where the game's assets are stored")
#ifdef _WIN32
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
#else
        .default_value(std::string("/mnt/c/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
#endif

    parser.add_argument("--by-magic", "-m")
        .help("Export by magic")
        .nargs(0)
        .default_value(false)
        .implicit_value(true);

    parser.add_argument("--raw", "-r")
        .help("Export raw data")
        .nargs(0)
        .default_value(false)
        .implicit_value(true);
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser("export", WARPGATE_VERSION);
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

    logger::info("export: loading assets (using synthium {})", synthium::version());
    std::string server = parser.get<std::string>("--assets-directory");
    std::string input_filename = parser.get<std::string>("asset_name");
    std::vector<std::filesystem::path> paths = glob::glob({
        server + "assets_x64_*.pack2",
        server + "data_x64_*.pack2"
    });

    size_t zone_loc;
    bool chunk_file = input_filename.find(".cnk") != std::string::npos;
    if(
        (zone_loc = input_filename.find(".zone")) != std::string::npos 
        || (chunk_file && ((zone_loc = input_filename.find("_")) != std::string::npos))
    ) {
        std::vector<std::filesystem::path> additional = glob::glob({
            server + input_filename.substr(0, zone_loc) + "_x64_*.pack2"
        });
        paths.insert(paths.end(), additional.begin(), additional.end());
    }

    synthium::Manager manager(paths);

    if(!manager.contains(input_filename)) {
        logger::error("{} not found in loaded assets", input_filename);
        std::exit(1);
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

    bool by_magic = parser.get<bool>("--by-magic");
    bool raw = parser.get<bool>("--raw");
    if(by_magic) {
        size_t curr_pos = 0;
        size_t pos = input_filename.find_first_of("\\x");
        std::string magic;
        while(pos != std::string::npos) {
            magic += input_filename.substr(curr_pos, pos - curr_pos);
            std::string hex_str = input_filename.substr(pos + 2, 2);
            std::istringstream iss(hex_str);
            int hex_val;
            iss >> std::hex >> hex_val;
            char hex_byte = static_cast<char>(hex_val);
            magic += hex_byte;
            curr_pos = pos + 4;
            pos = input_filename.find_first_of("\\x", curr_pos);
        }
        if(curr_pos < input_filename.size()) {
            magic += input_filename.substr(curr_pos, input_filename.size() - curr_pos);
        }
        output_filename = std::filesystem::weakly_canonical(output_filename);
        if(std::filesystem::exists(output_filename) && !std::filesystem::is_directory(output_filename)) {
            logger::error("{} exists and is not a directory! Cannot export files to location.", output_filename.string());
            return 1;
        } else if(!std::filesystem::exists(output_filename)) {
            std::filesystem::create_directories(output_filename);
        }
        logger::info("Exporting by magic: {}", input_filename);
        std::span<uint8_t> magic_span((uint8_t*)magic.data(), magic.size());
        manager.export_by_magic(magic_span, output_filename);
        logger::info("Done.");
        return 0;
    }

    if(raw) {
        logger::info("Exporting raw data");
    }

    std::vector<uint8_t> data = manager.get(input_filename)->get_data(raw);
    std::unique_ptr<uint8_t[]> decompressed;
    std::span<uint8_t> data_span(data);
    if(chunk_file) {
        warpgate::chunk::Chunk chunk(data);
        logger::info("Decompressing chunk '{}' of size {} (Compressed size: {})", input_filename, synthium::utils::human_bytes(chunk.decompressed_size()), synthium::utils::human_bytes(chunk.compressed_size()));
        decompressed = chunk.decompress();
        data_span = std::span<uint8_t>(decompressed.get(), chunk.decompressed_size());
    }
    std::ofstream output(output_filename, std::ios::binary);
    output.write((char*)data_span.data(), data_span.size());
    output.close();
    logger::info("Wrote {} to {}", synthium::utils::human_bytes(data_span.size()), output_filename.string());
    return 0;
}