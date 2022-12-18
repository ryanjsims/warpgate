#include <fstream>
#include <sstream>

#include "version.h"

#include "argparse/argparse.hpp"
#include <glob/glob.h>
#include "synthium/synthium.h"

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
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));

    parser.add_argument("--by-magic", "-m")
        .help("Export by magic")
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
    synthium::Manager manager(
        glob::glob(
            server + "assets_x64_*.pack2"
        )
    );

    std::string input_filename = parser.get<std::string>("asset_name");
    std::filesystem::path output_filename(parser.get<std::string>("output_file"));

    bool by_magic = parser.get<bool>("--by-magic");
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

    std::vector<uint8_t> data = manager.get(input_filename)->get_data();
    std::ofstream output("export" / output_filename, std::ios::binary);
    output.write((char*)data.data(), data.size());
    output.close();
    logger::info("Wrote {} bytes to export/{}", data.size(), output_filename.string());
    return 0;
}