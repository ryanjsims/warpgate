#include "zone.h"

#include <spdlog/spdlog.h>

namespace logger = spdlog;
using namespace warpgate::zone;

Zone::Zone(std::span<uint8_t> subspan): buf_(subspan) {
    ZoneVersionHeader header = get<ZoneVersionHeader>(0);
    if(header.magic() != "ZONE") {
        logger::error("Not a Zone file (got magic {})", header.magic());
        throw std::invalid_argument("Not a Zone file");
    }
    version_ = header.version;

    uint32_t offset = ecos_offset() + sizeof(uint32_t);
    uint32_t count = eco_count();
    for(uint32_t i = 0; i < count; i++) {
        std::shared_ptr<Eco> eco = std::make_shared<Eco>(buf_.subspan(offset));
        ecos.push_back(eco);
        offset += (uint32_t)eco->size();
    }

    offset = floras_offset() + sizeof(uint32_t);
    count = flora_count();
    for(uint32_t i = 0; i < count; i++) {
        std::shared_ptr<Flora> flora = std::make_shared<Flora>(buf_.subspan(offset), version_);
        floras.push_back(flora);
        offset += (uint32_t)flora->size();
    }

    offset = invis_walls_offset() + sizeof(uint32_t);
    count = invis_walls_count();
    for(uint32_t i = 0; i < count; i++) {
        logger::error("Invis walls are not actually implemented by this loader - send this Zone file to ryanjsims on github for investigation!");
        std::exit(1);
        // std::shared_ptr<InvisWall> invis_wall = std::make_shared<InvisWall>(buf_.subspan(offset));
        // invis_walls.push_back(invis_wall);
        // offset += invis_wall->size();
    }

    offset = objects_offset() + sizeof(uint32_t);
    count = objects_count();
    for(uint32_t i = 0; i < count; i++) {
        std::shared_ptr<RuntimeObject> object = std::make_shared<RuntimeObject>(buf_.subspan(offset), version_);
        objects.push_back(object);
        offset += (uint32_t)object->size();
    }

    offset = lights_offset() + sizeof(uint32_t);
    count = lights_count();
    for(uint32_t i = 0; i < count; i++) {
        std::shared_ptr<Light> light = std::make_shared<Light>(buf_.subspan(offset));
        lights.push_back(light);
        offset += (uint32_t)light->size();
    }
}

Zone::ref<ZoneHeader> Zone::header() const {
    if(version_ > 3) {
        logger::warn("Zone::header called on Zone v{} file", version_);
    }
    return get<ZoneHeader>(0);
}

Zone::ref<ZoneHeaderv45> Zone::header_v45() const {
    if(version_ < 4) {
        logger::warn("Zone::header_v45 called on Zone v{} file", version_);
    }
    return get<ZoneHeaderv45>(0);
}

Zone::ref<uint32_t> Zone::eco_count() const {
    return get<uint32_t>(ecos_offset());
}

std::shared_ptr<Eco> Zone::eco(uint32_t index) const {
    return ecos.at(index);
}


Zone::ref<uint32_t> Zone::flora_count() const {
    return get<uint32_t>(floras_offset());
}

std::shared_ptr<Flora> Zone::flora(uint32_t index) const {
    return floras.at(index);
}


Zone::ref<uint32_t> Zone::invis_walls_count() const {
    return get<uint32_t>(invis_walls_offset());
}

std::shared_ptr<InvisWall> Zone::invis_wall(uint32_t index) const {
    return invis_walls.at(index);
}


Zone::ref<uint32_t> Zone::objects_count() const {
    return get<uint32_t>(objects_offset());
}

std::shared_ptr<RuntimeObject> Zone::object(uint32_t index) const {
    return objects.at(index);
}


Zone::ref<uint32_t> Zone::lights_count() const {
    return get<uint32_t>(lights_offset());
}

std::shared_ptr<Light> Zone::light(uint32_t index) const {
    return lights.at(index);
}


Zone::ref<uint32_t> Zone::unknowns_count() const {
    return get<uint32_t>(unknowns_offset());
}

std::span<uint8_t> Zone::unknown_data() const {
    return buf_.subspan(unknowns_offset() + sizeof(uint32_t));
}

Zone::ref<uint32_t> Zone::decals_count() const {
    return get<uint32_t>(decals_offset());
}

uint32_t Zone::ecos_offset() const {
    if(version_ > 3) {
        ZoneHeaderv45 header = header_v45();
        return header.offsets.ecos;
    }
    ZoneHeader header = this->header();
    return header.offsets.ecos;
}

uint32_t Zone::floras_offset() const {
    if(version_ > 3) {
        ZoneHeaderv45 header = header_v45();
        return header.offsets.floras;
    }
    ZoneHeader header = this->header();
    return header.offsets.floras;
}

uint32_t Zone::invis_walls_offset() const {
    if(version_ > 3) {
        ZoneHeaderv45 header = header_v45();
        return header.offsets.invis_walls;
    }
    ZoneHeader header = this->header();
    return header.offsets.invis_walls;
}

uint32_t Zone::objects_offset() const {
    if(version_ > 3) {
        ZoneHeaderv45 header = header_v45();
        return header.offsets.objects;
    }
    ZoneHeader header = this->header();
    return header.offsets.objects;
}

uint32_t Zone::lights_offset() const {
    if(version_ > 3) {
        ZoneHeaderv45 header = header_v45();
        return header.offsets.lights;
    }
    ZoneHeader header = this->header();
    return header.offsets.lights;
}

uint32_t Zone::unknowns_offset() const {
    if(version_ > 3) {
        ZoneHeaderv45 header = header_v45();
        return header.offsets.unknown;
    }
    ZoneHeader header = this->header();
    return header.offsets.unknown;
}

uint32_t Zone::decals_offset() const {
    if(version_ < 4) {
        throw std::out_of_range(std::string("Zone v") + std::to_string(version_) + " does not have decals");
    }
    ZoneHeaderv45 header = header_v45();
    return header.offsets.decals;
}
