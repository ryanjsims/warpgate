#include "jenkins.h"
#include <cstring>

namespace jenkins {
    uint32_t lookup2(const char* key, uint32_t init) {
        size_t length = std::strlen(key);
        size_t position = length;
        std::shared_ptr<uint32_t> a(new uint32_t), b(new uint32_t), c(new uint32_t);
        uint32_t p = 0;
        *a = 0x9e3779b9;
        *b = *a;
        *c = init;
        while(position >= 12) {
            *a += key[p+0] + (((uint32_t)key[p+1]) << 8) + (((uint32_t)key[p+2]) << 16) + (((uint32_t)key[p+3]) << 24);
            *b += key[p+4] + (((uint32_t)key[p+5]) << 8) + (((uint32_t)key[p+6]) << 16) + (((uint32_t)key[p+7]) << 24);
            *c += key[p+8] + (((uint32_t)key[p+9]) << 8) + (((uint32_t)key[p+10]) << 16) + (((uint32_t)key[p+11]) << 24);
            mix(a, b, c);
            p += 12;
            position -= 12;
        }
        *c += length;
        if(position >= 11) *c += ((uint32_t)key[p+10])<<24;
        if(position >= 10) *c += ((uint32_t)key[p+9])<<16;
        if(position >= 9) *c += ((uint32_t)key[p+8])<<8;
        if(position >= 8) *b += ((uint32_t)key[p+7])<<24;
        if(position >= 7) *b += ((uint32_t)key[p+6])<<16;
        if(position >= 6) *b += ((uint32_t)key[p+5])<<8;
        if(position >= 5) *b += ((uint32_t)key[p+4]);
        if(position >= 4) *a += ((uint32_t)key[p+3])<<24;
        if(position >= 3) *a += ((uint32_t)key[p+2])<<16;
        if(position >= 2) *a += ((uint32_t)key[p+1])<<8;
        if(position >= 1) *a += ((uint32_t)key[p+0]);

        mix(a, b, c);
        return *c;
    }

    void mix(std::shared_ptr<uint32_t> a, std::shared_ptr<uint32_t> b, std::shared_ptr<uint32_t> c) {
        *a -= *b;
        *a -= *c;
        *a ^= (*c) >> 13;

        *b -= *c;
        *b -= *a;
        *b ^= (*a) << 8;

        *c -= *a;
        *c -= *b;
        *c ^= (*b) >> 13;

        *a -= *b;
        *a -= *c;
        *a ^= (*c) >> 12;

        *b -= *c;
        *b -= *a;
        *b ^= (*a) << 16;
        
        *c -= *a;
        *c -= *b;
        *c ^= (*b) >> 5;
        
        *a -= *b;
        *a -= *c;
        *a ^= (*c) >> 3;
        
        *b -= *c;
        *b -= *a;
        *b ^= (*a) << 10;
        
        *c -= *a;
        *c -= *b;
        *c ^= (*b) >> 15;
    }

    // Note: Forgelight uses a signed 32 bit int when hashing, but returns it as unsigned
    uint32_t oaat(const char* key) {
        size_t length = strlen(key);
        size_t i = 0;
        int32_t hash = 0;
        while(i < length) {
            hash += (signed char)key[i];
            hash += hash << 10;
            hash ^= hash >> 6;
            i++;
        }
        hash += hash << 3;
        hash ^= hash >> 11;
        hash += hash << 15;
        return (uint32_t)hash;
    }
}