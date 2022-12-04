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

    enum class Semantic {
        BASE_COLOR = -1295769314,
        NORMAL_MAP = 789998085,
        SPECULAR = 67211600,
        DETAIL_SELECT = 1716414136,
        DETAIL_CUBE = -125639093,
        OVERLAY0 = -1260182040,
        OVERLAY1 = 1449224430,
        BASE_CAMO = -305773271,
        UNKNOWN = 0
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
    
    ref<int32_t> semantic_hash() const;
    ref<D3DXParamClass> _class() const;
    ref<D3DXParamType> type() const;
    ref<uint32_t> length() const;
    std::span<uint8_t> data() const;
    static std::string semantic_name(int32_t semantic);
    static std::string semantic_name(Semantic semantic);
};

//std::string semantic_name(int32_t semantic);