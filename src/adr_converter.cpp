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
#include "utils/adr.h"
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
    synthium::Manager& manager, 
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
        case Parameter::Semantic::BASE_COLOR1:
        case Parameter::Semantic::BASE_COLOR2:
        case Parameter::Semantic::BASE_COLOR3:
        case Parameter::Semantic::BASE_COLOR4:
        case Parameter::Semantic::EMISSIVE1:
        case Parameter::Semantic::BASE_CAMO:
        case Parameter::Semantic::DETAIL_SELECT:
        case Parameter::Semantic::OVERLAY0:
        case Parameter::Semantic::OVERLAY1:
            utils::textures::save_texture(texture_name, manager.get(texture_name)->get_data(), output_directory);
            break;
        case Parameter::Semantic::NORMAL_MAP1:
        case Parameter::Semantic::NORMAL_MAP2:
            utils::textures::process_normalmap(texture_name, manager.get(texture_name)->get_data(), output_directory);
            break;
        case Parameter::Semantic::SPECULAR1:
        case Parameter::Semantic::SPECULAR2:
        case Parameter::Semantic::SPECULAR3:
        case Parameter::Semantic::SPECULAR4:
            albedo_name = texture_name;
            index = albedo_name.find_last_of('_');
            albedo_name[index + 1] = 'C';
            if(manager.contains(albedo_name)) {
                utils::textures::process_specular(texture_name, manager.get(texture_name)->get_data(), manager.get(albedo_name)->get_data(), output_directory);
            }
            break;
        case Parameter::Semantic::DETAIL_CUBE1:
        case Parameter::Semantic::DETAIL_CUBE2:
            utils::textures::process_detailcube(texture_name, manager.get(texture_name)->get_data(), output_directory);
            break;
        default:
            logger::warn("Skipping unimplemented semantic: {}", texture_name);
            break;
        }
    }
}

void build_argument_parser(argparse::ArgumentParser &parser, int &log_level) {
    parser.add_description("C++ ADR to GLTF2 model conversion tool");
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
        .help("Exclude textures from the output. Creates a monochromatic material instead.")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    parser.add_argument("--assets-directory", "-d")
        .help("The directory where the game's assets are stored")
#ifdef _WIN32
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
#else
        .default_value(std::string("/mnt/c/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
#endif   
    parser.add_argument("--rigify", "-r")
        .help("Export bones named to match bones generated by Rigify (for humanoid rigs)")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);
    
}

void load_asset(synthium::Manager &manager, std::string input_str, std::vector<uint8_t> &data_vector, std::span<uint8_t> &data_span, std::shared_ptr<uint8_t[]> &data) {
    std::filesystem::path input_filename(input_str);
    if(manager.contains(input_str)) {
        logger::debug("Loading '{}' from manager...", input_str);
        int retries = 3;
        std::shared_ptr<synthium::Asset2> asset = manager.get(input_str);
        //manager.deallocate(asset->uncompressed_size());
        while(data_vector.size() == 0 && retries > 0) {
            try {
                data_vector = asset->get_data();
                data_span = std::span<uint8_t>(data_vector.data(), data_vector.size());
                logger::debug("Loaded '{}' from manager.", input_str);
            } catch(std::bad_alloc) {
                logger::warn("Failed to load asset, deallocating some packs");
                manager.deallocate(asset->uncompressed_size());
            } catch(std::exception &err) {
                logger::error("Failed to load '{}' from manager: {}", input_str, err.what());
                std::exit(1);
            }
            retries--;
        }
    } else {
        logger::debug("Loading '{}' from filesystem...", input_str);
        std::ifstream input(input_filename, std::ios::binary | std::ios::ate);
        if(input.fail()) {
            logger::error("Failed to open file '{}'", input_filename.string());
            std::exit(2);
        }
        size_t length = input.tellg();
        input.seekg(0);
        data = std::make_shared<uint8_t[]>(length);
        input.read((char*)data.get(), length);
        input.close();
        data_span = std::span<uint8_t>(data.get(), length);
        logger::debug("Loaded '{}' from filesystem.", input_str);
    }
}

int main(int argc, const char* argv[]) {
    argparse::ArgumentParser parser("adr_converter", WARPGATE_VERSION);
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
    
    logger::info("Converting file {} using adr_converter {}", input_str, WARPGATE_VERSION);
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

    std::shared_ptr<uint8_t[]> data;
    std::vector<uint8_t> data_vector;
    std::span<uint8_t> data_span;

    load_asset(manager, input_str, data_vector, data_span, data);
    
    std::filesystem::path output_filename(parser.get<std::string>("output_file"));
    output_filename = std::filesystem::weakly_canonical(output_filename);
    std::filesystem::path output_directory;
    if(output_filename.has_parent_path()) {
        output_directory = output_filename.parent_path();
    }

    try {
        if(!std::filesystem::exists(output_filename.parent_path())) {
            logger::debug("Creating directories '{}'...", (output_directory / "textures").string());
            std::filesystem::create_directories(output_directory / "textures");
            logger::debug("Created directories '{}'.", (output_directory / "textures").string());
        }
    } catch (std::filesystem::filesystem_error& err) {
        logger::error("Failed to create directory {}: {}", err.path1().string(), err.what());
        std::exit(3);
    }

    utils::tsqueue<std::pair<std::string, Parameter::Semantic>> image_queue;

    std::string format = parser.get<std::string>("--format");
    bool include_skeleton = !parser.get<bool>("--no-skeleton");
    bool export_textures = !parser.get<bool>("--no-textures");
    bool rigify_skeleton = parser.get<bool>("--rigify");

    std::vector<std::thread> image_processor_pool;
    if(export_textures) {
        logger::info("Using {} image processing thread{}", image_processor_thread_count, image_processor_thread_count == 1 ? "" : "s");
        for(uint32_t i = 0; i < image_processor_thread_count; i++) {
            image_processor_pool.push_back(std::thread{
                process_images, 
                std::ref(manager), 
                std::ref(image_queue), 
                output_directory
            });
        }
    } else {
        logger::info("Not exporting textures by user request.");
    }

    utils::ADR adr(data_span);
    std::optional<std::string> dme_file = adr.base_model();
    if(!dme_file) {
        std::exit(1);
    }

    std::optional<std::string> dmat_file = adr.base_palette();
    std::shared_ptr<DMAT> dmat = nullptr;
    std::shared_ptr<uint8_t[]> dmat_data;
    std::vector<uint8_t> dmat_data_vector;
    std::span<uint8_t> dmat_data_span;
    if(dmat_file) {
        load_asset(manager, *dmat_file, dmat_data_vector, dmat_data_span, dmat_data);
        dmat.reset(new DMAT(dmat_data_span));
    }

    std::shared_ptr<uint8_t[]> dme_data;
    std::vector<uint8_t> dme_data_vector;
    std::span<uint8_t> dme_data_span;

    load_asset(manager, *dme_file, dme_data_vector, dme_data_span, dme_data);

    
    std::shared_ptr<DME> dme;
    if(dmat != nullptr) {
        dme.reset(new DME(dme_data_span, output_filename.stem().string(), dmat));
    } else {
        dme.reset(new DME(dme_data_span, output_filename.stem().string()));
    }
    tinygltf::Model gltf = utils::gltf::dme::build_gltf_from_dme(*dme, image_queue, output_directory, export_textures, include_skeleton, rigify_skeleton);
    
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