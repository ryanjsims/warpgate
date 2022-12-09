#include <chrono>
#include <fstream>
#include <filesystem>
#include <memory>
#include <thread>

#include "argparse/argparse.hpp"
#include "cnk_loader.h"
#include "dme_loader.h"
#include "zone_loader.h"
#include "utils/gltf/chunk.h"
#include "utils/adr.h"
#include "utils/textures.h"
#include "utils/tsqueue.h"
#include "synthium/synthium.h"
#include "tiny_gltf.h"
#include "version.h"

#include <glob/glob.h>
#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace logger = spdlog;


void process_images(
    const synthium::Manager& manager,
    warpgate::utils::tsqueue<
        std::tuple<
            std::string, 
            std::shared_ptr<uint8_t[]>, uint32_t, 
            std::shared_ptr<uint8_t[]>, uint32_t
        >
    >& chunk_queue,
    warpgate::utils::tsqueue<std::pair<std::string, warpgate::Parameter::Semantic>> &dme_queue,
    std::filesystem::path output_directory
) {
    logger::debug("Got output directory {}", output_directory.string());
    while(!chunk_queue.is_closed() || !dme_queue.is_closed()) {
        auto chunk_value = chunk_queue.try_dequeue_for(1ms);
        if(chunk_value) {
            auto[texture_basename, cnx_data, cnx_length, sny_data, sny_length] = *chunk_value;
            std::span<uint8_t> cnx_map(cnx_data.get(), cnx_length);
            std::span<uint8_t> sny_map(sny_data.get(), sny_length);
            warpgate::utils::textures::process_cnx_sny(texture_basename, cnx_map, sny_map, output_directory);
        }

        auto dme_value = dme_queue.try_dequeue_for(1ms);
        if(dme_value) {
            std::string albedo_name;
            size_t index;
            auto[texture_name, semantic] = *dme_value;
            switch (semantic)
            {
            case warpgate::Parameter::Semantic::BASE_COLOR:
            case warpgate::Parameter::Semantic::BASE_CAMO:
            case warpgate::Parameter::Semantic::DETAIL_SELECT:
            case warpgate::Parameter::Semantic::OVERLAY0:
            case warpgate::Parameter::Semantic::OVERLAY1:
                warpgate::utils::textures::save_texture(texture_name, manager.get(texture_name).get_data(), output_directory);
                break;
            case warpgate::Parameter::Semantic::NORMAL_MAP:
                warpgate::utils::textures::process_normalmap(texture_name, manager.get(texture_name).get_data(), output_directory);
                break;
            case warpgate::Parameter::Semantic::SPECULAR:
                albedo_name = texture_name;
                index = albedo_name.find_last_of('_');
                albedo_name[index + 1] = 'C';
                warpgate::utils::textures::process_specular(texture_name, manager.get(texture_name).get_data(), manager.get(albedo_name).get_data(), output_directory);
                break;
            case warpgate::Parameter::Semantic::DETAIL_CUBE:
                warpgate::utils::textures::process_detailcube(texture_name, manager.get(texture_name).get_data(), output_directory);
                break;
            default:
                logger::warn("Skipping unimplemented semantic: {}", texture_name);
                break;
            }
        }
    }
    logger::info("Both queues closed, stopping thread");
}

void build_argument_parser(argparse::ArgumentParser &parser, int &log_level) {
    parser.add_description("C++ Forgelight Chunk to GLTF2 model conversion tool");
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

    parser.add_argument("--no-textures", "-i")
        .help("Exclude the textures from the output")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    parser.add_argument("--assets-directory", "-d")
        .help("The directory where the game's assets are stored")
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
    
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser("zone_converter", WARPGATE_VERSION);
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
    
    logger::info("Converting file {} using dme_converter {}", input_str, WARPGATE_VERSION);

    std::string continent_name = std::filesystem::path(input_str).stem().string();

    std::string path = parser.get<std::string>("--assets-directory");
    std::filesystem::path server(path);
    std::vector<std::filesystem::path> packs = glob::glob({
        (server / (continent_name + "_x64_*.pack2")).string(),
        (server / "assets_x64_*.pack2").string()
    });

    logger::info("Loading {} packs...", packs.size());
    synthium::Manager manager(packs);
    logger::info("Manager loaded.");

    std::filesystem::path input_filename(input_str);
    std::unique_ptr<uint8_t[]> data;
    std::vector<uint8_t> data_vector, chunk1_data_vector;
    std::span<uint8_t> data_span, chunk1_data_span;
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

    std::string format = parser.get<std::string>("--format");
    bool export_textures = !parser.get<bool>("--no-textures");
    uint32_t image_processor_thread_count = parser.get<uint32_t>("--threads");
    // hmm
    warpgate::utils::tsqueue<
        std::tuple<
            std::string, 
            std::shared_ptr<uint8_t[]>, uint32_t, 
            std::shared_ptr<uint8_t[]>, uint32_t
        >
    > chunk_image_queue;

    warpgate::utils::tsqueue<std::pair<std::string, warpgate::Parameter::Semantic>> dme_image_queue;

    std::vector<std::thread> image_processor_pool;
    if(export_textures) {
        logger::info("Using {} image processing thread{}", image_processor_thread_count, image_processor_thread_count == 1 ? "" : "s");
        for(uint32_t i = 0; i < image_processor_thread_count; i++) {
            image_processor_pool.push_back(std::thread{
                process_images,
                std::cref(manager),
                std::ref(chunk_image_queue), 
                std::ref(dme_image_queue),
                output_directory
            });
        }
    } else {
        logger::info("Not exporting textures by user request.");
    }

    logger::info("Parsing zone...");
    warpgate::zone::Zone continent(data_span);
    logger::info("Parsed zone.");

    if(continent.version() > 3) {
        logger::error("Not currently set up for exporting ZONE version > 3 (got version {})", continent.version());
        return 1;
    }

    tinygltf::Model gltf;
    tinygltf::Sampler dme_sampler, chunk_sampler;
    int dme_sampler_index = (int)gltf.samplers.size();
    dme_sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    dme_sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    dme_sampler.wrapS = TINYGLTF_TEXTURE_WRAP_REPEAT;
    dme_sampler.wrapT = TINYGLTF_TEXTURE_WRAP_REPEAT;
    gltf.samplers.push_back(dme_sampler);

    int chunk_sampler_index = (int)gltf.samplers.size();
    chunk_sampler.magFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    chunk_sampler.minFilter = TINYGLTF_TEXTURE_FILTER_LINEAR;
    chunk_sampler.wrapS = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
    chunk_sampler.wrapT = TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE;
    gltf.samplers.push_back(chunk_sampler);
    
    gltf.defaultScene = (int)gltf.scenes.size();
    gltf.scenes.push_back({});

    std::unordered_map<uint32_t, uint32_t> texture_indices;
    std::unordered_map<uint32_t, std::vector<uint32_t>> material_indices;

    warpgate::zone::ZoneHeader header = continent.header();

    int terrain_parent_index = (int)gltf.nodes.size();
    tinygltf::Node terrain_parent;
    terrain_parent.name = "Terrain";
    gltf.nodes.push_back(terrain_parent);
    for(uint32_t x = 0; x < header.chunk_info.count_x; x += 4) {
        for(uint32_t y = 0; y < header.chunk_info.count_y; y += 4) {
            std::string chunk_stem = continent_name + "_" + std::to_string((int)(header.chunk_info.start_x + x)) + "_" + std::to_string((int)(header.chunk_info.start_y + y));
            std::unique_ptr<uint8_t[]> decompressed_cnk0_data, decompressed_cnk1_data;
            size_t cnk0_length, cnk1_length;
            {
                std::vector<uint8_t> chunk0_data = manager.get(std::filesystem::path(chunk_stem).replace_extension(".cnk0").string()).get_data();
                warpgate::chunk::Chunk compressed_chunk0(chunk0_data);
                decompressed_cnk0_data = std::move(compressed_chunk0.decompress());
                cnk0_length = compressed_chunk0.decompressed_size();
            }
            {
                std::vector<uint8_t> chunk1_data = manager.get(std::filesystem::path(chunk_stem).replace_extension(".cnk1").string()).get_data();
                warpgate::chunk::Chunk compressed_chunk1(chunk1_data);
                decompressed_cnk1_data = std::move(compressed_chunk1.decompress());
                cnk1_length = compressed_chunk1.decompressed_size();
            }
            warpgate::chunk::CNK0 cnk0({decompressed_cnk0_data.get(), cnk0_length});
            warpgate::chunk::CNK1 cnk1({decompressed_cnk1_data.get(), cnk1_length});
            int chunk_index = warpgate::utils::gltf::chunk::add_chunks_to_gltf(gltf, cnk0, cnk1, chunk_image_queue, output_directory, chunk_stem, chunk_sampler_index, export_textures);

            gltf.nodes.at(chunk_index).translation = {((int)(header.chunk_info.start_x + x)) * 64.0, 0.0, ((int)(header.chunk_info.start_y + y)) * 64.0};
            gltf.nodes.at(terrain_parent_index).children.push_back(chunk_index);
        }
    }

    gltf.asset.version = "2.0";
    gltf.asset.generator = "warpgate " + std::string(WARPGATE_VERSION) + " via tinygltf";

    logger::info("Writing GLTF2 file {}...", output_filename.filename().string());
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&gltf, output_filename.string(), false, format == "glb", format == "gltf", format == "glb");
    
    chunk_image_queue.close();
    dme_image_queue.close();
    logger::info("Joining image processing thread{}...", image_processor_pool.size() == 1 ? "" : "s");
    for(uint32_t i = 0; i < image_processor_pool.size(); i++) {
        image_processor_pool.at(i).join();
    }
    logger::info("Done.");
    return 0;
}