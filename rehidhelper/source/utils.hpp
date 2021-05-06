#pragma once
#include <string>
#include <3ds.h>
#include "download.hpp"
#include "ArduinoJson.h"
#include "json.hpp"

class Utils
{
    public:
        void DownloadAndExtractLatestReleaseZip();
        void CreateRehidJson(u64 tid, bool global, const std::string &data);
        std::string ParseJson(const std::string &data);
    private:
        Download m_download;
};