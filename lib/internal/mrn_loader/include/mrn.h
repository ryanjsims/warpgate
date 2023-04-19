#pragma once
#include <cstring>
#include <stdexcept>
#include <memory>
#include <span>
#include <vector>

#include "packet.h"

namespace warpgate::mrn {
    struct MRN {
        mutable std::span<uint8_t> buf_;

        MRN();
        MRN(std::span<uint8_t> subspan, std::string name);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("MRN: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        std::string name() const;
        std::vector<std::shared_ptr<Packet>> packets() const;
        std::shared_ptr<Packet> operator[](size_t index) const {
            return m_packets[index];
        }

        std::shared_ptr<SkeletonNamesPacket> skeleton_names() const;
        std::shared_ptr<FilenamesPacket> file_names() const;
        std::vector<uint32_t> skeleton_indices() const;

    private:
        std::vector<std::shared_ptr<Packet>> m_packets;
        std::string m_name;
        uint32_t m_skeleton_names_index, m_filenames_index;
        std::vector<uint32_t> m_skeleton_indices;
    };
}