#include <fstream>
#include <filesystem>
#include <memory>

#include "argparse/argparse.hpp"
#include "cnk_loader.h"
#include "utils/gltf/chunk.h"
#include "synthium/synthium.h"
#include "tiny_gltf.h"
#include "version.h"

#include <spdlog/spdlog.h>

namespace logger = spdlog;

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
    
    // parser.add_argument("--threads", "-t")
    //     .help("The number of threads to use for image processing")
    //     .default_value(4u)
    //     .scan<'u', uint32_t>();

    // parser.add_argument("--no-skeleton", "-s")
    //     .help("Exclude the skeleton from the output")
    //     .default_value(false)
    //     .implicit_value(true)
    //     .nargs(0);
    
    parser.add_argument("--no-textures", "-i")
        .help("Exclude the skeleton from the output")
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    parser.add_argument("--assets-directory", "-d")
        .help("The directory where the game's assets are stored")
        .default_value(std::string("C:/Users/Public/Daybreak Game Company/Installed Games/Planetside 2 Test/Resources/Assets/"));
    
}

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser("cnk_converter", CPPDMOD_VERSION);
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
    std::string path = parser.get<std::string>("--assets-directory");
    std::filesystem::path server(path);
    std::vector<std::filesystem::path> assets;
    for(int i = 0; i < 5; i++) {
        assets.push_back(server / ("Nexus_x64_" + std::to_string(i) + ".pack2"));
    }

    logger::info("Loading packs...");
    synthium::Manager manager(assets);
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

    if(input_filename.extension().string() == ".cnk0" && manager.contains(input_filename.filename().replace_extension("cnk1").string())) {
        chunk1_data_vector = manager.get(input_filename.filename().replace_extension("cnk1").string()).get_data();
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

    warpgate::Chunk compressed_chunk0(data_span);
    std::unique_ptr<uint8_t[]> decompressed_chunk0 = compressed_chunk0.decompress();

    warpgate::CNK0 chunk0({decompressed_chunk0.get(), compressed_chunk0.decompressed_size()});

    warpgate::Chunk compressed_chunk1(chunk1_data_span);
    std::unique_ptr<uint8_t[]> decompressed_chunk1 = compressed_chunk1.decompress();

    warpgate::CNK1 chunk1({decompressed_chunk1.get(), compressed_chunk1.decompressed_size()});
    for(uint32_t i = 0; i < chunk1.textures_count(); i++) {
        std::filesystem::path texture = output_directory / "textures" / ("color_nx_" + std::to_string(i) + ".dds");
        std::ofstream output(texture, std::ios::binary);
        if(output.fail()) {
            logger::error("Failed to open file '{}'", texture.string());
            break;
        }
        output.write((char*)chunk1.textures()[i].color_nx_map().data(), chunk1.textures()[i].color_length());
        output.close();
        logger::info("Wrote {}", texture.string());

        texture = output_directory / "textures" / ("spec_ny_" + std::to_string(i) + ".dds");
        output.open(texture, std::ios::binary);
        if(output.fail()) {
            logger::error("Failed to open file '{}'", texture.string());
            break;
        }
        output.write((char*)chunk1.textures()[i].specular_ny_map().data(), chunk1.textures()[i].specular_length());
        output.close();
        logger::info("Wrote {}", texture.string());

        logger::info("Extra1 has length {}", chunk1.textures()[i].extra1_length());
        logger::info("Extra2 has length {}", chunk1.textures()[i].extra2_length());
        logger::info("Extra3 has length {}", chunk1.textures()[i].extra3_length());
        logger::info("Extra4 has length {}", chunk1.textures()[i].extra4_length());
    }

    tinygltf::Model gltf;
    gltf.defaultScene = (int)gltf.scenes.size();
    gltf.scenes.push_back({});
    logger::info("Adding chunk to gltf...");
    warpgate::utils::gltf::chunk::add_mesh_to_gltf(gltf, chunk0, (int)gltf.materials.size(), input_filename.stem().string());
    logger::info("Added chunk to gltf");

    for(uint32_t i = 0; i < chunk0.tile_count(); i++) {
        if(chunk0.tiles().at(i).has_image()) {
            std::filesystem::path texture = output_directory / "textures" / ("tile_" + std::to_string(i) + ".dds");
            std::ofstream output(texture, std::ios::binary);
            if(output.fail()) {
                logger::error("Failed to open file '{}'", texture.string());
                break;
            }
            output.write((char*)chunk0.tiles().at(i).image_data().data(), chunk0.tiles().at(i).image_length());
            output.close();
            logger::info("Wrote {}", texture.string());
        }
    }

    logger::info("{} has {} textures", input_filename.filename().replace_extension("cnk1").string(), chunk1.textures_count());

    logger::info("Writing gltf file...");
    tinygltf::TinyGLTF writer;
    writer.WriteGltfSceneToFile(&gltf, output_filename.string(), false, format == "glb", format == "gltf", format == "glb");
    logger::info("Successfully wrote gltf file!");
    return 0;
}