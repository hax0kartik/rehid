#pragma once
#include <vector>
#include <map>
#include <utility>
#include <string>

namespace Utils{
    class JsonManager{
        public:
            void operator=(const JsonManager &) = delete;
            std::string ParseAndGetLatest(const std::vector<uint8_t> &data);
        private:
    };
}