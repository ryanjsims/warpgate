#include <fstream>
#include <filesystem>
#include <cmath>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <synthium/synthium.h>

#include "argparse/argparse.hpp"
#include "dme_loader.h"
#include "utils/gltf/dme.h"
#include "utils/materials_3.h"
#include "utils/textures.h"
#include "utils/tsqueue.h"
#include "utils.h"
#include "tiny_gltf.h"
#include "version.h"

namespace logger = spdlog;
using namespace warpgate;

void process_images(
    const synthium::Manager& manager, 
    utils::tsqueue<std::pair<std::string, Parameter::Semantic>>& queue, 
    std::filesystem::path output_directory
) {
    logger::debug("Got output directory {}", output_directory.string());
    while(!queue.is_closed()) {
        auto texture_info = queue.try_dequeue({"", Parameter::Semantic::UNKNOWN});
        std::string texture_name = texture_info.first, albedo_name;
        size_t index;
        Parameter::Semantic semantic = texture_info.second;
        if(semantic == Parameter::Semantic::UNKNOWN) {
            logger::info("Got default value from try_dequeue, stopping thread.");
            break;
        }

        switch (semantic)
        {
        case Parameter::Semantic::BASE_COLOR:
        case Parameter::Semantic::BASE_CAMO:
        case Parameter::Semantic::DETAIL_SELECT:
        case Parameter::Semantic::OVERLAY0:
        case Parameter::Semantic::OVERLAY1:
            utils::textures::save_texture(texture_name, manager.get(texture_name).get_data(), output_directory);
            break;
        case Parameter::Semantic::NORMAL_MAP:
            utils::textures::process_normalmap(texture_name, manager.get(texture_name).get_data(), output_directory);
            break;
        case Parameter::Semantic::SPECULAR:
            albedo_name = texture_name;
            index = albedo_name.find_last_of('_');
            albedo_name[index + 1] = 'C';
            utils::textures::process_specular(texture_name, manager.get(texture_name).get_data(), manager.get(albedo_name).get_data(), output_directory);
            break;
        case Parameter::Semantic::DETAIL_CUBE:
            utils::textures::process_detailcube(texture_name, manager.get(texture_name).get_data(), output_directory);
            break;
        default:
            logger::warn("Skipping unimplemented semantic: {}", texture_name);
            break;
        }
    }
}

void build_argument_parser(argparse::ArgumentParser &parser, int &log_level) {
    parser.add_description("C++ DMEv4 to GLTF2 model conversion tool");
    parser.add_argument("input_file");
    parser.add_argument("output_file");
    parser.add_argument("--format", "-f")
        .help("Select the output file format {glb, gltf}")
        .required()
        .action([](const std::string& value) {
            static const std::vector<std::string> choices = { "gltf", "glb" };
            if (std::find(choices.begin(), choices.end(), value) != choices.end()) {
                return value;
            }
            return std::string{ "" };
        });
    
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
    
    parser.add_argument("--threads", "-t")
        .help("The number of threads to use for image processing")
        .default_value(4u)
        .scan<'u', uint32_t>();

    parser.add_argument("--no-skeleton", "-s")
        .help("Exclude the skeleton from the output")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);
    
    parser.add_argument("--no-textures", "-i")
        .help("Exclude the skeleton from the output")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    parser.add_argument("--assets-directory", "-d")
        .help("The directory where the game's assets are stored")
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
    
}

int main(int argc, const char* argv[]) {
    argparse::ArgumentParser parser("dme_converter", CPPDMOD_VERSION);
    int log_level = logger::level::warn;

    build_argument_parser(parser, log_level);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        std::exit(1);
    }
    logger::set_level(logger::level::level_enum(log_level));

    std::string input_str = parser.get<std::string>("input_file");
    
    logger::info("Converting file {} using dme_converter {}", input_str, CPPDMOD_VERSION);
    uint32_t image_processor_thread_count = parser.get<uint32_t>("--threads");
    std::string path = parser.get<std::string>("--assets-directory");
    std::filesystem::path server(path);
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 24; i++) {
        assets.push_back(server / ("assets_x64_" + std::to_string(i) + ".pack2"));
    }
    
    logger::info("Loading packs...");
    synthium::Manager manager(assets);
    logger::info("Manager loaded.");
    
    logger::info("Loading materials.json");
    utils::materials3::init_materials();
    logger::info("Loaded materials.json");

    std::filesystem::path input_filename(input_str);
    std::unique_ptr<uint8_t[]> data;
    std::vector<uint8_t> data_vector;
    std::span<uint8_t> data_span;
    if(manager.contains(input_str)) {
        data_vector = manager.get(input_str).get_data();
        data_span = std::span<uint8_t>(data_vector.data(), data_vector.size());
    } else {
        std::ifstream input(input_filename, std::ios::binary | std::ios::ate);
        if(input.fail()) {
            logger::error("Failed to open file '{}'", input_filename.string());
            std::exit(2);
        }
        size_t length = input.tellg();
        input.seekg(0);
        data = std::make_unique<uint8_t[]>(length);
        input.read((char*)data.get(), length);
        input.close();
        data_span = std::span<uint8_t>(data.get(), length);
    }
    
    std::filesystem::path output_filename(parser.get<std::string>("output_file"));
    output_filename = std::filesystem::weakly_canonical(output_filename);
    std::filesystem::path output_directory;
    if(output_filename.has_parent_path()) {
        output_directory = output_filename.parent_path();
    }

    try {
        if(!std::filesystem::exists(output_filename.parent_path())) {
            std::filesystem::create_directories(output_directory / "textures");
        }
    } catch (std::filesystem::filesystem_error& err) {
        logger::error("Failed to create directory {}: {}", err.path1().string(), err.what());
        std::exit(3);
    }

    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> image_queue;

    std::string format = parser.get<std::string>("--format");
    bool include_skeleton = !parser.get<bool>("--no-skeleton");
    bool export_textures = !parser.get<bool>("--no-textures");

    std::vector<std::thread> image_processor_pool;
    if(export_textures) {
        logger::info("Using {} image processing thread{}", image_processor_thread_count, image_processor_thread_count == 1 ? "" : "s");
        for(uint32_t i = 0; i < image_processor_thread_count; i++) {
            image_processor_pool.push_back(std::thread{
                process_images, 
                std::cref(manager), 
                std::ref(image_queue), 
                output_directory
            });
        }
    } else {
        logger::info("Not exporting textures by user request.");
    }

    DME dme(data_span, output_filename.stem().string());
    tinygltf::Model gltf = utils::gltf::dme::build_gltf_from_dme(dme, image_queue, output_directory, export_textures, include_skeleton);
    
    logger::info("Writing GLTF2 file {}...", output_filename.filename().string());
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&gltf, output_filename.string(), false, format == "glb", format == "gltf", format == "glb");
    
    image_queue.close();
    logger::info("Joining image processing thread{}...", image_processor_pool.size() == 1 ? "" : "s");
    for(uint32_t i = 0; i < image_processor_pool.size(); i++) {
        image_processor_pool.at(i).join();
    }
    logger::info("Done.");
    return 0;
}
