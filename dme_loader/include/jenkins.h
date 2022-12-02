#include <cstdint>
#include <memory>
#include <string>

namespace jenkins {
    uint32_t lookup2(const char* key, uint32_t init = 0);

    /**
     * @brief Performs the jenkins hash mix operation on the integers contained by a, b, and c. Modifies all three.
     * 
     * @param a 
     * @param b 
     * @param c 
     */
    void mix(std::shared_ptr<uint32_t> a, std::shared_ptr<uint32_t> b, std::shared_ptr<uint32_t> c);
    // Note: Forgelight uses a signed 32 bit int when hashing, but returns it as unsigned
    uint32_t oaat(const char* key);
    uint32_t oaat(std::string key);
};