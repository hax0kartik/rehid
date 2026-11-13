#include "JsonManager.hpp"
#include "ArduinoJson.h"

std::pair<std::string, std::string> Utils::JsonManager::ParseAndGetLatest(const std::vector<uint8_t> &data) {
    DynamicJsonDocument doc(1 * 1024 * 1024);
    deserializeJson(doc, (const char*)data.data(), data.size());
    std::string url = doc["assets"][0]["browser_download_url"];
    std::string sha256 = doc["assets"][0]["digest"];
    return {url, sha256};
}
