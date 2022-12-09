#include "utils/adr.h"

#include <spdlog/spdlog.h>

using namespace warpgate;
using namespace pugi;

namespace logger = spdlog;

utils::ADR::ADR(std::shared_ptr<uint8_t[]> data, std::size_t size): data_(data) {
    xml_parse_result result = document.load_buffer_inplace(data_.get(), size);
    if(!result) {
        logger::error("Failed to load ADR file from buffer: {}", result.description());
        throw std::runtime_error(result.description());
    }

    root = document.child("ActorRuntime");

    if(!root) {
        logger::error("Not an ActorRuntime XML document");
        throw std::invalid_argument("Not an ActorRuntime XML document");
    }
}

utils::ADR::ADR(std::span<uint8_t> data_span) {
    data_ = std::make_shared<uint8_t[]>(data_span.size());
    std::memcpy(data_.get(), data_span.data(), data_span.size());
    xml_parse_result result = document.load_buffer_inplace(data_.get(), data_span.size());
    if(!result) {
        logger::error("Failed to load ADR file from buffer: {}", result.description());
        throw std::runtime_error(result.description());
    }

    root = document.child("ActorRuntime");

    if(!root) {
        logger::error("Not an ActorRuntime XML document");
        throw std::invalid_argument("Not an ActorRuntime XML document");
    }
}
    
std::optional<std::string> utils::ADR::base_model() {
    xml_node base = root.child("Base");
    if(!base) {
        logger::error("Base node not found.");
        return {};
    }

    xml_attribute fileName = base.attribute("fileName");
    if(!fileName) {
        logger::error("Base node did not have a fileName attribute!");
        return {};
    }
    return std::string(fileName.as_string());
}

std::optional<std::string> utils::ADR::base_palette() {
    xml_node base = root.child("Base");
    if(!base) {
        logger::error("Base node not found.");
        return {};
    }

    xml_attribute paletteName = base.attribute("paletteName");
    if(!paletteName) {
        logger::error("Base node did not have a paletteName attribute!");
        return {};
    }
    return std::string(paletteName.as_string());
}

std::optional<std::string> utils::ADR::animation_network() {
    xml_node animationNetwork = root.child("AnimationNetwork");
    if(!animationNetwork) {
        logger::error("AnimationNetwork node not found.");
        return {};
    }

    xml_attribute fileName = animationNetwork.attribute("fileName");
    if(!fileName) {
        logger::error("AnimationNetwork node did not have a fileName attribute!");
        return {};
    }
    return std::string(fileName.as_string());
}

std::optional<std::string> utils::ADR::animation_set() {
    xml_node animationNetwork = root.child("AnimationNetwork");
    if(!animationNetwork) {
        logger::error("AnimationNetwork node not found.");
        return {};
    }

    xml_attribute animSetName = animationNetwork.attribute("animSetName");
    if(!animSetName) {
        logger::error("AnimationNetwork node did not have a animSetName attribute!");
        return {};
    }
    return std::string(animSetName.as_string());
}