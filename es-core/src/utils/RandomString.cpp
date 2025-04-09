#include "utils/RandomString.h"
#include <random>
#include <algorithm>

namespace Utils {
    namespace String {
        std::string generateRandom(size_t length) {
            static const char alphanum[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
            std::string str(length, 0);
            std::generate_n(str.begin(), length, [&]() { return alphanum[dis(gen)]; });
            return str;
        }
    }
}