#include "runtime_object.h"

using namespace warpgate::zone;

RuntimeObject::RuntimeObject(std::span<uint8_t> subspan, uint32_t version): buf_(subspan) {
    actor_file_ = std::string((char*)buf_.data());
    uint32_t offset = (uint32_t)(sizeof(float) + sizeof(uint32_t) + actor_file_.size() + 1);
    uint32_t instance_count = this->instance_count();
    for(uint32_t i = 0; i < instance_count; i++) {
        Instance instance(buf_.subspan(offset), version);
        instances.push_back(instance);
        offset += (uint32_t)instance.size();
    }
    buf_ = buf_.first(offset);
}

std::string RuntimeObject::actor_file() const {
    return actor_file_;
}

RuntimeObject::ref<float> RuntimeObject::render_distance() const {
    return get<float>(actor_file_.size() + 1);
}

RuntimeObject::ref<uint32_t> RuntimeObject::instance_count() const {
    return get<uint32_t>(actor_file_.size() + 1 + sizeof(float));
}


Instance RuntimeObject::instance(uint32_t index) const {
    return instances.at(index);
}
