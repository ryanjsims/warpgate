#pragma once
#include <memory>
#include <span>
#include <vector>

#include "structs.h"
#include "eco.h"
#include "flora.h"
#include "runtime_object.h"


namespace warpgate {
    struct Zone {
        mutable std::span<uint8_t> buf_;

        Zone(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Zone: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<ZoneHeader> header() const;
        ref<uint32_t> eco_count() const;
        std::shared_ptr<Eco> eco(uint32_t index) const;

        ref<uint32_t> flora_count() const;
        std::shared_ptr<Flora> flora(uint32_t index) const;

        ref<uint32_t> invis_walls_count() const;
        std::shared_ptr<InvisWall> invis_wall(uint32_t index) const;

        ref<uint32_t> objects_count() const;

    
    private:
        std::vector<std::shared_ptr<Eco>> ecos;
        std::vector<std::shared_ptr<Flora>> floras;
        std::vector<std::shared_ptr<InvisWall>> invis_walls;
        std::vector<std::shared_ptr<RuntimeObject>> objects;

    };
}