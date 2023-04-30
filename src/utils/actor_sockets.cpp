#include "utils/actor_sockets.h"

#include <spdlog/spdlog.h>

#if __cpp_lib_shared_ptr_arrays < 201707L
#error warpgate::utils::ActorSockets requires a compiler that supports std::make_shared<T[]> (__cpp_lib_shared_ptr_arrays >= 201707L)
#endif

using namespace warpgate::utils;
using namespace pugi;

namespace logger = spdlog;

ActorSockets::ActorSockets(std::shared_ptr<uint8_t[]> data, std::size_t size) {
    logger::info("Loading ActorSockets in place...");
    xml_parse_result result = document.load_buffer_inplace(data_.get(), size);
    if(!result) {
        logger::error("Failed to load ActorSockets.xml from buffer: {}", result.description());
        throw std::runtime_error(result.description());
    }

    root = document.child("ActorSockets");

    if(!root) {
        logger::error("Not an ActorSockets XML document");
        throw std::invalid_argument("Not an ActorSockets XML document");
    }

    init();
}

ActorSockets::ActorSockets(std::span<uint8_t> data) {
    logger::info("Loading ActorSockets...");

    data_ = std::make_shared<uint8_t[]>(data.size());
    std::memcpy(data_.get(), data.data(), data.size());
    xml_parse_result result = document.load_buffer_inplace(data_.get(), data.size());
    if(!result) {
        logger::error("Failed to load ActorSockets.xml from buffer: {}", result.description());
        throw std::runtime_error(result.description());
    }

    root = document.child("ActorSockets");

    if(!root) {
        logger::error("Not an ActorSockets XML document");
        throw std::invalid_argument("Not an ActorSockets XML document");
    }

    init();
}

void ActorSockets::init() {
    xml_node traversal_node = root.child("SkeletalModels");
    if(!traversal_node) {
        logger::warn("SkeletalModels not found in ActorSockets.xml");
    } else {
        for(auto it = traversal_node.begin(); it != traversal_node.end(); it++) {
            SkeletalModel model(*it);
            if(model.name.has_value()) {
                model_indices[*model.name] = static_cast<uint32_t>(skeletal_models.size());
            }
            skeletal_models.push_back(model);
        }
    }

    traversal_node = root.child("SkeletalNetworks");
    if(!traversal_node) {
        logger::warn("SkeletalNetworks not found in ActorSockets.xml");
    } else {
        for(auto it = traversal_node.begin(); it != traversal_node.end(); it++) {
            SkeletalNetwork network(*it);
            if(network.name.has_value()) {
                network_indices[*network.name] = static_cast<uint32_t>(skeletal_networks.size());
            }
            skeletal_networks.push_back(network);
        }
    }
    logger::info("Loaded ActorSockets.");
}

SkeletalNetwork::SkeletalNetwork(xml_node node) {
    logger::debug("Loading SkeletalNetwork...");
    xml_attribute name_attr = node.attribute("name");
    if(name_attr) {
        name = name_attr.as_string();
        logger::debug("Network name {}", *name);
    }
    xml_node skeletons_node = node.child("Skeletons");
    if(!skeletons_node) {
        logger::warn("Skeletons not found in SkeletalNetwork '{}'", name.has_value() ? *name : "<unnamed>");
    } else {
        for(auto it = skeletons_node.begin(); it != skeletons_node.end(); it++) {
            SkeletalModel model(*it);
            if(model.name.has_value()) {
                skeleton_indices[*model.name] = static_cast<uint32_t>(skeletons.size());
            }
            skeletons.push_back(model);
        }
    }
    logger::debug("Loaded SkeletalNetwork with {} skeletons", skeletons.size());
}

SkeletalModel::SkeletalModel(xml_node node) {
    logger::debug("Loading {}...", node.name());
    xml_attribute name_attr = node.attribute("name");
    if(name_attr) {
        name = name_attr.as_string();
        logger::debug("{} name {}", node.name(), *name);
    }
    xml_node sockets_node = node.child("Sockets");

    if(!sockets_node) {
        logger::warn("Sockets not found in {} '{}'", node.name(), name.has_value() ? *name : "<unnamed>");
    } else {
        for(auto it = sockets_node.begin(); it != sockets_node.end(); it++) {
            AttachSocket socket(*it);
            sockets.push_back(socket);
        }
    }
    logger::debug("Loaded {} with {} sockets", node.name(), sockets.size());
}

AttachSocket::AttachSocket(xml_node node) {
    xml_attribute name_attr = node.attribute("name");
    if(name_attr) {
        name = name_attr.as_string();
    }

    xml_attribute bone_attr = node.attribute("bone");
    if(bone_attr) {
        bone = bone_attr.as_string();
    }

    xml_attribute heading_attr = node.attribute("heading");
    xml_attribute pitch_attr = node.attribute("pitch");
    xml_attribute roll_attr = node.attribute("roll");

    glm::vec3 rotvec;

    if(heading_attr) {
        rotvec.y = glm::radians(heading_attr.as_float());
    }

    if(pitch_attr) {
        rotvec.x = glm::radians(pitch_attr.as_float());
    }

    if(roll_attr) {
        rotvec.z = glm::radians(roll_attr.as_float());
    }

    if(heading_attr || pitch_attr || roll_attr) {
        rotation = glm::quat(rotvec);
    }

    xml_attribute offsetx_attr = node.attribute("offsetX");
    xml_attribute offsety_attr = node.attribute("offsetY");
    xml_attribute offsetz_attr = node.attribute("offsetZ");

    if(offsetx_attr || offsety_attr || offsetz_attr) {
        offset = glm::vec3();
    }

    if(offsetx_attr) {
        offset->x = offsetx_attr.as_float();
    }

    if(offsety_attr) {
        offset->y = offsety_attr.as_float();
    }

    if(offsetz_attr) {
        offset->z = offsetz_attr.as_float();
    }

    xml_attribute scalex_attr = node.attribute("scaleX");
    xml_attribute scaley_attr = node.attribute("scaleY");
    xml_attribute scalez_attr = node.attribute("scaleZ");

    if(scalex_attr || scaley_attr || scalez_attr) {
        scale = glm::vec3();
    }

    if(scalex_attr) {
        scale->x = scalex_attr.as_float();
    }

    if(scaley_attr) {
        scale->y = scaley_attr.as_float();
    }

    if(scalez_attr) {
        scale->z = scalez_attr.as_float();
    }
}