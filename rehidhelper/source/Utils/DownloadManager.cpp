#include "DownloadManager.hpp"
#include "picounzip.hpp"

static size_t write_function(void *ptr, size_t size, size_t nmemb, void *stream) {
    std::vector<uint8_t> *data = (std::vector<uint8_t> *)stream;
    uint8_t *u8data = (uint8_t*)ptr;

    for (int i = 0; i < (int)(size * nmemb); i++)
        data->push_back(u8data[i]);

    return size * nmemb;
}

static size_t file_write_function(void *ptr, size_t size, size_t nmemb, void *stream) {
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

void Utils::DownloadManager::Intialize() {
    m_handle = curl_easy_init();
    curl_easy_setopt(m_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_handle, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(m_handle, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_handle, CURLOPT_USERAGENT, "Wumiibo");
    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &m_data);
}

int Utils::DownloadManager::GetUrl(const std::string &url, std::vector<uint8_t> &data, bool write_to_file, FILE *f, long timeout) {
    data.clear();
    data.shrink_to_fit();
    m_data.clear();
    m_data.shrink_to_fit();

    m_url = url;
    curl_easy_setopt(m_handle, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &m_data);
    curl_easy_setopt(m_handle, CURLOPT_TIMEOUT, timeout);

    if (write_to_file) {
        curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, file_write_function);
        curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, f);
    }

    int res = curl_easy_perform(m_handle);

    if (res == 0 && !write_to_file)
        data = m_data;

    return res;
}

void Utils::DownloadManager::DownloadAndUnzipTo(const std::string &url, const std::string &location) {
    std::vector<uint8_t> data;
    mkdir("/3ds/rehid", 0777);

    FILE *file = fopen("/3ds/rehid/download.zip", "wb+");

    if (file) {
        GetUrl(url, data, true, file);
    }

    fclose(file);
    picounzip::unzip zip("/3ds/rehid/download.zip");
    zip.extractall(location);
}

bool Utils::DownloadManager::DownloadTo(const std::string &url, const std::string &location, const std::string &filename, const std::string &sha256) {
    std::vector<uint8_t> data;

    //This silently fails when location is already present
    mkdir(location.c_str(), 0777);

    const std::string finalloc = location + "/" + filename;
    const int res = GetUrl(url, data, false, nullptr, 90);
    if (res != 0)
        return false;

    if (data.empty() || data.size() == 0)
        return false;

    FILE *file = fopen(finalloc.c_str(), "wb+");
    if (!file)
        return false;

    fwrite(data.data(), data.size(), 1, file);
    const bool writeError = fflush(file) != 0;
    const long filesize = fseek(file, 0, SEEK_END) == 0 ? ftell(file) : -1L;
    const int closeResult = fclose(file);

    if (writeError || closeResult != 0 || filesize < 0 || static_cast<size_t>(filesize) != data.size()) {
        remove(finalloc.c_str());
        return false;
    }

    if (!sha256.empty()) {
        const bool verified = verifySha256(finalloc, sha256);
        if (!verified) {
            // SHA256 verification failed, removing the downloaded file
            remove(finalloc.c_str());
            return false;
        }
    }

    return true;
}

bool Utils::DownloadManager::verifySha256(const std::string &file, const std::string &sha256) {
    const size_t offset = sha256.compare(0, 7, "sha256:") == 0 ? 7 : 0;
    if (sha256.size() != offset + 64)
        return false;

    for (size_t i = offset; i < sha256.size(); ++i) {
        if (!std::isxdigit(static_cast<unsigned char>(sha256[i])))
            return false;
    }

    FILE *input = fopen(file.c_str(), "rb");
    if (!input)
        return false;

    mbedtls_sha256_context context;
    mbedtls_sha256_init(&context);
    int result = mbedtls_sha256_starts_ret(&context, 0);
    unsigned char buffer[4096];
    size_t bytesRead;
    while (result == 0 && (bytesRead = fread(buffer, 1, sizeof(buffer), input)) > 0)
        result = mbedtls_sha256_update_ret(&context, buffer, bytesRead);

    const bool readError = ferror(input) != 0;
    const int closeResult = fclose(input);
    unsigned char digest[32];
    if (result == 0 && !readError && closeResult == 0)
        result = mbedtls_sha256_finish_ret(&context, digest);
    mbedtls_sha256_free(&context);

    if (result != 0 || readError || closeResult != 0)
        return false;

    static const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < sizeof(digest); ++i) {
        if (std::tolower(static_cast<unsigned char>(sha256[offset + 2 * i])) != hex[digest[i] >> 4] ||
            std::tolower(static_cast<unsigned char>(sha256[offset + 2 * i + 1])) != hex[digest[i] & 0x0f])
            return false;
    }
    return true;
}
