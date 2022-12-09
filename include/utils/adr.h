#pragma once

#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

#include <pugixml.hpp>

namespace warpgate::utils {
    struct ADR {
        /**
         * Note: Mutates data while loading the XML document in place.
         */
        ADR(std::shared_ptr<uint8_t[]> data, std::size_t size);

        /**
         * Makes a copy of data before loading
         */
        ADR(std::span<uint8_t> data);
    
        std::optional<std::string> base_model();
        std::optional<std::string> base_palette();
        std::optional<std::string> animation_network();
        std::optional<std::string> animation_set();

    private:
        pugi::xml_document document;
        pugi::xml_node root;
        std::shared_ptr<uint8_t[]> data_;
    };
}