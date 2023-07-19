#include "DownloadManager.hpp"
#include "picounzip.hpp"

static size_t write_function(void *ptr, size_t size, size_t nmemb, void *stream){
    std::vector<uint8_t> *data = (std::vector<uint8_t> *)stream;
    uint8_t *u8data = (uint8_t*)ptr;
    for(int i = 0; i < (int)(size * nmemb); i++)
        data->push_back(u8data[i]);
    return size * nmemb;
}

static size_t file_write_function(void *ptr, size_t size, size_t nmemb, void *stream){
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

void Utils::DownloadManager::Intialize(){
    m_handle = curl_easy_init();
    curl_easy_setopt(m_handle, CURLOPT_VERBOSE, 0L);
    curl_easy_setopt(m_handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_handle, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(m_handle, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(m_handle, CURLOPT_USERAGENT, "Wumiibo");
    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &m_data);
}

int Utils::DownloadManager::GetUrl(const std::string &url, std::vector<uint8_t> &data, bool write_to_file, FILE *f){
    data.clear();
    data.shrink_to_fit();
    m_data.clear();
    m_data.shrink_to_fit();

    m_url = url;
    curl_easy_setopt(m_handle, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, write_function);
    curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, &m_data);
    if(write_to_file){
        curl_easy_setopt(m_handle, CURLOPT_WRITEFUNCTION, file_write_function);
        curl_easy_setopt(m_handle, CURLOPT_WRITEDATA, f);
    }
    int res = curl_easy_perform(m_handle);
    if(res == 0 && !write_to_file)
        data = m_data;
    return res;
}

void Utils::DownloadManager::DownloadAndUnzipTo(const std::string &url, const std::string &location){
    std::vector<uint8_t> data;
    mkdir("/3ds/rehid", 0777);

    FILE *file = fopen("/3ds/rehid/download.zip", "wb+");
    if(file){
        GetUrl(url, data, true, file);
    }
    fclose(file);
    picounzip::unzip zip("/3ds/rehid/download.zip");
    zip.extractall(location);
}

void Utils::DownloadManager::DownloadTo(const std::string &url, const std::string &location, const std::string &filename){
    std::vector<uint8_t> data;

    //This silently fails when location is already present
    mkdir(location.c_str(), 0777);

    const std::string finalloc = location + "/" + filename;
    GetUrl(url, data);
    FILE *file = fopen(finalloc.c_str(), "wb+");
    fwrite(data.data(), data.size(), 1, file);
    fclose(file);
}
