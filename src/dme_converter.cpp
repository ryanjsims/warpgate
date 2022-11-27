#include <fstream>
#include <filesystem>
#include <memory>

#include <spdlog/spdlog.h>
#include "argparse/argparse.hpp"
#include "dme_loader.h"
#include "tiny_gltf.h"
#include "json.hpp"



int main(int argc, const char* argv[]) {
    argparse::ArgumentParser parser("dme_converter", CPP_DMOD_VERSION);
    parser.add_description("C++ DMEv4 to GLTF2 model conversion tool");
    parser.add_argument("input_file");
    parser.add_argument("output_file");
    parser.add_argument("--format", "-f")
        .help("Select the format in which to save the file {glb, gltf}")
        .required()
        .action([](const std::string& value) {
            static const std::vector<std::string> choices = { "gltf", "glb" };
            if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
                return value;
            }
            return std::string{ "" };
        });
    int log_level = spdlog::level::warn;
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
    
    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
    spdlog::set_level(spdlog::level::level_enum(log_level));
    std::filesystem::path input_filename(parser.get<std::string>("input_file"));
    
    std::filesystem::path output_filename(parser.get<std::string>("output_file"));
    output_filename = std::filesystem::weakly_canonical(output_filename);
    try {
        if(output_filename.has_parent_path() && !std::filesystem::exists(output_filename.parent_path())) {
            std::filesystem::create_directories(output_filename.parent_path());
        }
    } catch (std::filesystem::filesystem_error& err) {
        spdlog::error("Failed to create directory {}: {}", err.path1().string(), err.what());
        std::exit(3);
    }

    std::ifstream input(input_filename, std::ios::binary | std::ios::ate);
    if(input.fail()) {
        spdlog::error("Failed to open file '{}'", input_filename.string());
        std::exit(2);
    }
    size_t length = input.tellg();
    input.seekg(0);
    std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(length);
    input.read((char*)data.get(), length);

    DME dme(data.get(), length);
    tinygltf::Model gltf;

    return 0;
}