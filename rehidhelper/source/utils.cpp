#include <iostream>
#include <algorithm>
#include <sstream>
#include "picounzip.hpp"
#include "utils.hpp"
#include "config.hpp"

void Utils::DownloadAndExtractLatestReleaseZip()
{
    std::vector<uint8_t> tmp;
    m_download.GetUrl("https://api.github.com/repos/hax0kartik/rehid/releases/latest", tmp);
    DynamicJsonDocument doc(1 * 1024 * 1024);
    deserializeJson(doc, (const char*)tmp.data(), tmp.size());

    const char *url = doc["assets"][0]["browser_download_url"];
    std::string surl = (url);
    m_download.GetUrl(surl, tmp);
    FILE *f = fopen("/download.zip", "wb+");
    fwrite(tmp.data(), tmp.size(), 1, f);
    fclose(f);
    picounzip::unzip zip("/download.zip");
    zip.extractall("/luma/titles/");
    LumaConfig::EnableGamePatching();
}

void Utils::CreateRehidJson(u64 tid, bool global, const std::string &data)
{
    std::string loc = "/rehid/";
    mkdir(loc.c_str(), 0777);
    std::string fileloc;
    char hex[20];
    if(global)
        fileloc = loc.substr(0, loc.size() - 1);
    else
    {
        sprintf(hex, "%016llX", tid);
        fileloc = loc + hex;
        mkdir(fileloc.c_str(), 0777);
    }
    fileloc = fileloc + "/rehid.json";
    FILE *f = fopen(fileloc.c_str(), "wb+");
    fwrite(data.c_str(), 1, data.size(), f);
    fclose(f);
}

std::string Utils::ParseJson(const std::string &data)
{
    std::string str = "Remappings:\n";
    nlohmann::json m_j = nlohmann::json::parse(data);
    std::string get, press;
    if(m_j.contains("dpadtocpad"))
    {
        if(m_j["dpadtocpad"])
            str += "\ue006 > \ue077\n";
    }

    if(m_j.contains("cpadtodpad"))
    {
        if(m_j["cpadtodpad"])
            str += "\ue077 > \ue006\n";
    }

    if(m_j.contains("keys"))
    {
        for(int i = 0; i < (int)m_j["keys"].size(); i++)
        {
            str += m_j["keys"][i]["press"];
            str += " > ";
            str += m_j["keys"][i]["get"];
            str += "\n";
        }
    }

    if(m_j.contains("touch"))
    {
        for(int i = 0; i < (int)m_j["touch"].size(); i++)
        {
            str += m_j["touch"][i]["press"];
            str += " > ";
            str += "Touch: X:" + std::to_string(m_j["touch"][i]["get"][0].get<int>()) + ", Y:" +  std::to_string(m_j["touch"][i]["get"][1].get<int>());
            str += "\n";
        }
    }

    if(m_j.contains("cpad"))
    {
        for(int i = 0; i < (int)m_j["cpad"].size(); i++)
        {
            str += m_j["cpad"][i]["press"];
            str += " > ";
            str += "CirclePad: X:" + std::to_string(m_j["cpad"][i]["get"][0].get<int>()) + ", Y:" +  std::to_string(m_j["cpad"][i]["get"][1].get<int>());
            str += "\n";
        }
    }

    return str;
}