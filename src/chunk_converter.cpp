#include <fstream>
#include <filesystem>
#include <memory>
#include <thread>

#include "argparse/argparse.hpp"
#include "cnk_loader.h"
#include "utils/gltf/chunk.h"
#include "utils/textures.h"
#include "utils/tsqueue.h"
#include "synthium/synthium.h"
#include "tiny_gltf.h"
#include "version.h"

#include <glob/glob.h>
#include <spdlog/spdlog.h>

namespace logger = spdlog;

void process_images(
    warpgate::utils::tsqueue<
        std::tuple<
            std::string, 
            std::shared_ptr<uint8_t[]>, uint32_t, 
            std::shared_ptr<uint8_t[]>, uint32_t
        >
    >& queue, 
    std::filesystem::path output_directory
) {
    logger::debug("Got output directory {}", output_directory.string());
    while(!queue.is_closed()) {
        auto[texture_basename, cnx_data, cnx_length, sny_data, sny_length] = queue.try_dequeue({"", {}, 0, {}, 0});
        if(texture_basename == "") {
            logger::info("Got default value from try_dequeue, stopping thread.");
            break;
        }
        std::span<uint8_t> cnx_map(cnx_data.get(), cnx_length);
        std::span<uint8_t> sny_map(sny_data.get(), sny_length);
        warpgate::utils::textures::process_cnx_sny(texture_basename, cnx_map, sny_map, output_directory);
    }
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
#ifdef _WIN32
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
#else
        .default_value(std::string("/mnt/c/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
#endif    
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser("cnk_converter", WARPGATE_VERSION);
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
   
    std::string continent_name = input_str.substr(0, input_str.find("_"));

    std::string path = parser.get<std::string>("--assets-directory");
    std::filesystem::path server(path);
    std::vector<std::filesystem::path> packs = glob::glob({
        (server / (continent_name + "_x64_*.pack2")).string(),
    });

    logger::info("Loading packs...");
    synthium::Manager manager(packs);
    logger::info("Manager loaded.");

    std::filesystem::path input_filename(input_str);
    std::unique_ptr<uint8_t[]> data;
    std::vector<uint8_t> data_vector, chunk1_data_vector;
    std::span<uint8_t> data_span, chunk1_data_span;
    if(manager.contains(input_str)) {
        data_vector = manager.get(input_str)->get_data();
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

    if(input_filename.extension().string() == ".cnk0" && manager.contains(input_filename.filename().replace_extension("cnk1").string())) {
        chunk1_data_vector = manager.get(input_filename.filename().replace_extension("cnk1").string())->get_data();
        chunk1_data_span = std::span<uint8_t>(chunk1_data_vector.data(), chunk1_data_vector.size());
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
    > image_queue;

    std::vector<std::thread> image_processor_pool;
    if(export_textures) {
        logger::info("Using {} image processing thread{}", image_processor_thread_count, image_processor_thread_count == 1 ? "" : "s");
        for(uint32_t i = 0; i < image_processor_thread_count; i++) {
            image_processor_pool.push_back(std::thread{
                process_images, 
                std::ref(image_queue), 
                output_directory
            });
        }
    } else {
        logger::info("Not exporting textures by user request.");
    }

    warpgate::chunk::Chunk compressed_chunk0(data_span);
    std::unique_ptr<uint8_t[]> decompressed_chunk0 = compressed_chunk0.decompress();

    warpgate::chunk::CNK0 chunk0({decompressed_chunk0.get(), compressed_chunk0.decompressed_size()});

    warpgate::chunk::Chunk compressed_chunk1(chunk1_data_span);
    std::unique_ptr<uint8_t[]> decompressed_chunk1 = compressed_chunk1.decompress();

    warpgate::chunk::CNK1 chunk1({decompressed_chunk1.get(), compressed_chunk1.decompressed_size()});

    logger::info("Adding chunk to gltf...");
    tinygltf::Model gltf = warpgate::utils::gltf::chunk::build_gltf_from_chunks(chunk0, chunk1, output_directory, export_textures, image_queue, input_filename.stem().string());
    logger::info("Added chunk to gltf");

    logger::info("Writing gltf file...");
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&gltf, output_filename.string(), false, format == "glb", format == "gltf", format == "glb");
    logger::info("Successfully wrote gltf file!");

    image_queue.close();
    logger::info("Joining image processing thread{}...", image_processor_pool.size() == 1 ? "" : "s");
    for(uint32_t i = 0; i < image_processor_pool.size(); i++) {
        image_processor_pool.at(i).join();
    }
    logger::info("Done.");
    return 0;
}