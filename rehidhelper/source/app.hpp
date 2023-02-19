#pragma once
#include <string>
#include "ui.hpp"
#include "States/Initial.hpp"
#include "States/MainMenu.hpp"
#include "States/Download.hpp"
#include "States/Camera.hpp"
#include "States/GameSelection.hpp"
#include "States/Toggle.hpp"
#include "Utils/TitleManager.hpp"
#include "Utils/JsonManager.hpp"
#include "Utils/DownloadManager.hpp"

class App{
    public:
        void Intialize();
        void RunLoop();
        void Finalize();
        void ChangeState(auto newstate);
        auto& GetTitleManager(){ return m_titlemanager; };
        auto& GetJsonManager(){ return m_jsonmanager; };
        auto& GetDownloadManager(){ return m_downloadmanager; };
        void SetTitle(uint64_t tid) { m_currenttitle = tid; };
        uint64_t GetTitle() { return m_currenttitle; };
        bool IsConnected() { return m_connected; };
        bool IsReboot() { return m_reboot; };
        auto& GetPayload() { return m_payload; };
    private:
        Initial m_initial;
        MainMenu m_mainmenu;
        Download m_download;
        Camera m_camera;
        GameSelection m_gameselection;
        Toggle m_toggle;
        ui::State *m_currstate = nullptr;
        Utils::TitleManager m_titlemanager;
        Utils::JsonManager m_jsonmanager;
        Utils::DownloadManager m_downloadmanager;

        std::string m_payload;
        uint64_t m_currenttitle = 0;
        bool m_connected = false;
        bool m_reboot = false;
};