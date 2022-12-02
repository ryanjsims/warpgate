#pragma once
#include <span>
#include <stdexcept>

struct Parameter {
    enum class D3DXParamType {
        VOID,
        BOOL,
        INT,
        FLOAT,
        STRING,
        TEXTURE,
        TEXTURE1D,
        TEXTURE2D,
        TEXTURE3D,
        TEXTURECUBE,
        SAMPLER,
        SAMPLER1D,
        SAMPLER2D,
        SAMPLER3D,
        SAMPLERCUBE,
        PIXELSHADER,
        VERTEXSHADER,
        PIXELFRAGMENT,
        VERTEXFRAGMENT,
        UNSUPPORTED,
        FORCE_DWORD = 0x7fffffff
    };

    enum class D3DXParamClass {
        SCALAR,
        VECTOR,
        MATRIX_ROWS,
        MATRIX_COLS,
        OBJECT,
        STRUCT,
        FORCE_DWORD = 0x7fffffff
    };

    mutable std::span<uint8_t> buf_;

    Parameter(std::span<uint8_t> subspan);

    template <typename T>
    struct ref {
        uint8_t * const p_;
        ref (uint8_t *p) : p_(p) {}
        operator T () const { T t; memcpy(&t, p_, sizeof(t)); return t; }
        T operator = (T t) const { memcpy(p_, &t, sizeof(t)); return t; }
    };

    template <typename T>
    ref<T> get (size_t offset) const {
        if (offset + sizeof(T) > buf_.size()) throw std::out_of_range("Parameter: Offset out of range");
        return ref<T>(&buf_[0] + offset);
    }

    size_t size() const {
        return buf_.size();
    }
    
    ref<uint32_t> namehash() const;
    ref<D3DXParamClass> _class() const;
    ref<D3DXParamType> type() const;
    ref<uint32_t> length() const;
    std::span<uint8_t> data() const;
};