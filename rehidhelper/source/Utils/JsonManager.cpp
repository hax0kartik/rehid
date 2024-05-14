#include "JsonManager.hpp"
#include "ArduinoJson.h"

std::string Utils::JsonManager::ParseAndGetLatest(const std::vector<uint8_t> &data) {
    DynamicJsonDocument doc(1 * 1024 * 1024);
    deserializeJson(doc, (const char*)data.data(), data.size());
    std::string url = doc["assets"][0]["browser_download_url"];
    return url;
}