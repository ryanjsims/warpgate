#include "utils.h"
#include <algorithm>

std::string utils::uppercase(const std::string input) {
    std::string temp = input;
    std::transform(temp.begin(), temp.end(), temp.begin(), [](const char value) -> char {
        if(value >= 'a' && value <= 'z') {
            return value - (char)('a' - 'A');
        }
        return value;
    });
    return temp;
}