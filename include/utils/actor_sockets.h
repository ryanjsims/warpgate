#pragma once

#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

#include <pugixml.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

namespace warpgate::utils {
    struct AttachSocket {
        AttachSocket(pugi::xml_node node);

        std::optional<std::string> name;
        std::optional<std::string> bone;
        std::optional<glm::vec3> offset;
        std::optional<glm::vec3> scale;
        std::optional<glm::quat> rotation;
    };

    struct SkeletalModel {
        SkeletalModel(pugi::xml_node node);

        std::optional<std::string> name;
        std::vector<AttachSocket> sockets;
    };

    struct SkeletalNetwork {
        SkeletalNetwork(pugi::xml_node node);

        std::optional<std::string> name;
        std::vector<SkeletalModel> skeletons;
        std::unordered_map<std::string, uint32_t> skeleton_indices;
    };

    struct ActorSockets {
        /**
         * Note: Mutates data while loading the XML document in place.
         */
        ActorSockets(std::shared_ptr<uint8_t[]> data, std::size_t size);

        /**
         * Makes a copy of data before loading
         */
        ActorSockets(std::span<uint8_t> data);
    
        std::vector<SkeletalModel> skeletal_models;
        std::vector<SkeletalNetwork> skeletal_networks;
        std::unordered_map<std::string, uint32_t> model_indices, network_indices;

    private:
        pugi::xml_document document;
        pugi::xml_node root;
        std::shared_ptr<uint8_t[]> data_;

        void init();
    };
}