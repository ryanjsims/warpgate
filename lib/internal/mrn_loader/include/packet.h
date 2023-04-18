#pragma once
#include <cstring>
#include <stdexcept>
#include <memory>
#include <span>

#include "packet_types.h"
#include "skeleton_data.h"
#include "file_data.h"
#include "nsa_file.h"

namespace warpgate::mrn {
    struct Header {
        mutable std::span<uint8_t> buf_;

        Header(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Header: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        ref<uint64_t> magic() const;
        ref<PacketType> type() const;
        ref<uint32_t> index() const;
        ref<uint32_t> namehash() const {
            return index();
        }

        std::span<uint32_t, 4> unknown_array() const;
        ref<uint32_t> data_length() const;
        ref<uint32_t> alignment() const;

    };

    struct Packet {
        mutable std::span<uint8_t> buf_;

        Packet(std::span<uint8_t> subspan);

        template <typename T>
        struct ref {
            uint8_t * const p_;
            ref (uint8_t *p) : p_(p) {}
            operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
            T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
        };

        template <typename T>
        ref<T> get (size_t offset) const {
            if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Packet: Offset out of range");
            return ref<T>(&buf_[0] + offset);
        }

        size_t size() const {
            return buf_.size();
        }

        std::shared_ptr<const Header> header() const;
        std::span<uint8_t> data() const;

    private:
        std::shared_ptr<Header> m_header = nullptr;
    };

    struct SkeletonPacket : Packet {
        SkeletonPacket(std::shared_ptr<Packet> packet);
        SkeletonPacket(std::span<uint8_t> subspan);

        std::shared_ptr<SkeletonData> skeleton() const;
    private:
        std::shared_ptr<SkeletonData> m_skeleton;
    };

    struct FilenamesPacket : Packet {
        FilenamesPacket(std::shared_ptr<Packet> packet);
        FilenamesPacket(std::span<uint8_t> subspan);

        std::shared_ptr<FileData> files() const;
    private:
        std::shared_ptr<FileData> m_files;
    };

    struct SkeletonNamesPacket : Packet {
        SkeletonNamesPacket(std::shared_ptr<Packet> packet);
        SkeletonNamesPacket(std::span<uint8_t> subspan);

        std::shared_ptr<ExpandedStringTable> skeleton_names() const;
    private:
        std::shared_ptr<ExpandedStringTable> m_skeleton_names;

        ref<uint64_t> skeleton_names_ptr() const;
    };

    struct NSAFilePacket : Packet {
        NSAFilePacket(std::shared_ptr<Packet> packet);
        NSAFilePacket(std::span<uint8_t> subspan);

        std::shared_ptr<NSAFile> animation() const;
    private:
        std::shared_ptr<NSAFile> m_animation;
    };
}