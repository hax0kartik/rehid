#pragma once
#include <functional>
#include <vector>
#include <string>
#include <3ds.h>
#include <curl/curl.h>
#include <mbedtls/sha256.h>

namespace Utils {
class DownloadManager {
public:
    DownloadManager() = default;
    void operator=(const DownloadManager &) = delete;
    void Intialize();
    int GetUrl(const std::string &url, std::vector<uint8_t> &data, bool write_to_file = false, FILE *f = nullptr, long timeout = 20);
    void DownloadAndUnzipTo(const std::string &url, const std::string &location);
    bool DownloadTo(const std::string &url, const std::string &location, const std::string &filename, const std::string &sha256 = "");
private:
    bool verifySha256(const std::string &file, const std::string &sha256);
    CURL *m_handle;
    std::string m_url;
    std::vector<uint8_t> m_data;
};
}
