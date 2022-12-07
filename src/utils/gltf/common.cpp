#include "utils/gltf/common.h"

// Here it is
#include "utils/sign.h"

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace warpgate;

int utils::gltf::add_texture_to_gltf(
    tinygltf::Model &gltf, 
    std::filesystem::path texture_path, 
    std::filesystem::path output_directory,
    int sampler,
    std::optional<std::string> label
) {
    int index = (int)gltf.textures.size();
    tinygltf::Texture tex;
    tex.source = (int)gltf.images.size();
    tex.name = label ? *label : texture_path.filename().string();
    tex.sampler = 0;
    tinygltf::Image img;
    img.uri = texture_path.lexically_relative(output_directory).string();
    
    gltf.textures.push_back(tex);
    gltf.images.push_back(img);
    return index;
}

void utils::gltf::update_bone_transforms(tinygltf::Model &gltf, int skeleton_root) {
    for(int child : gltf.nodes.at(skeleton_root).children) {
        update_bone_transforms(gltf, child);
        glm::dvec3 parent_translation(
            gltf.nodes.at(skeleton_root).translation[0], 
            gltf.nodes.at(skeleton_root).translation[1], 
            gltf.nodes.at(skeleton_root).translation[2]
        );
        glm::dquat parent_rotation(
            gltf.nodes.at(skeleton_root).rotation[3],
            gltf.nodes.at(skeleton_root).rotation[0], 
            gltf.nodes.at(skeleton_root).rotation[1], 
            gltf.nodes.at(skeleton_root).rotation[2]
        );
        
        glm::dvec3 child_translation = glm::dvec3(
            gltf.nodes.at(child).translation[0], 
            gltf.nodes.at(child).translation[1], 
            gltf.nodes.at(child).translation[2]
        );
        glm::dquat child_rotation(
            gltf.nodes.at(child).rotation[3],
            gltf.nodes.at(child).rotation[0], 
            gltf.nodes.at(child).rotation[1], 
            gltf.nodes.at(child).rotation[2]
        );

        child_translation -= parent_translation;
        
        child_rotation = glm::inverse(parent_rotation) * child_rotation;
        child_translation = utils::sign(parent_rotation) * glm::inverse(parent_rotation) * child_translation;


        gltf.nodes.at(child).translation = {
            child_translation.x, 
            child_translation.y, 
            child_translation.z
        };

        gltf.nodes.at(child).rotation = {
            child_rotation.x,
            child_rotation.y,
            child_rotation.z,
            child_rotation.w
        };
    }
}