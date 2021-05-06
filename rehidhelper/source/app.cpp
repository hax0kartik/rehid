#include <vector>
#include <string>
#include "app.hpp"
#include "draw.hpp"

void App::DoStuffBeforeMain()
{
    LightLock_Init(&m_toplock);
    LightLock_Init(&m_botlock);
    ui.top_func = std::bind(Draw::DrawLoadingScreen, &m_toplock);
    m_titles.PopulateTitleArray();
    m_titles.FilterOutTWLAndUpdate();
    m_titles.PopulateSMDHArray();
    m_titles.ConvertSMDHsToC2D();
}

void App::MainLoop()
{
    std::vector<std::string> options;
    options.push_back("Download Rehid");
    options.push_back("Scan QR code");
    //options.push_back("View Remap");

    std::vector<std::string> desc;
    desc.push_back("Description: Downloads and installs latest rehid.");
    desc.push_back("Description: Scan QR codes to generate remappings.");
    //desc.push_back("Description: View the remappings for a game.");
    std::string str;
    while(aptMainLoop())
    {
        hidScanInput();

        if(keysDown() & KEY_START)
        { 
            ui.bot_func = ui.top_func = nullptr;
            break;
        }
        if(m_state == 0)
        {
            if(ui.bot_func == nullptr)
            {
                ui.top_func = std::bind(Draw::DrawMainMenuTop, &m_toplock, m_image, &m_selected, desc);
                ui.bot_func = std::bind(Draw::DrawMainMenu, &m_botlock, &m_selected, options);
            }

            LightLock_Lock(&m_botlock);

            if(keysDown() & KEY_DOWN)
                m_selected++;
            if(keysDown() & KEY_UP)
                m_selected--;
            
            if(keysDown() & KEY_A)
            {
                ui.bot_func = nullptr;
                m_state = m_selected + 1;
            }
            
            if(m_selected < 0)
                m_selected = options.size() - 1;
            
            else if(m_selected > options.size() - 1)
                m_selected = 0;
            LightLock_Unlock(&m_botlock);
        }
        else if(m_state == 1)
        {
            if(ui.bot_func == nullptr)
            {
                if(!m_connected)
                {
                    str = "Not Connected to Internet. Press B.";
                    ui.bot_func = std::bind(Draw::DrawTextInCentre, false, &m_botlock, &str);
                    continue;
                }
                str = "Downloading latest release...";
                ui.bot_func = std::bind(Draw::DrawLoadingBarAndText, &m_botlock, &str);
                ui.top_func = std::bind(Draw::DrawLoadingScreen, &m_toplock);
                m_utils.DownloadAndExtractLatestReleaseZip();
                str = "Downloaded!\nYou'll need to restart to apply the changes.\nPress B.";
                ui.bot_func = std::bind(Draw::DrawTextInCentre, false, &m_botlock, &str);
            }

            if(keysDown() & KEY_B)
            {
                ui.bot_func = nullptr;
                m_state = 0;
            }
        }
        else if(m_state == 2)
        {
            if(ui.bot_func == nullptr)
            {
                m_camera.Initialize(&m_titles);
                m_camera.HandOverUI();
                m_camera.CreateCameraThread();
                m_camera.Controls();
                m_camera.Finalize();
                m_state = 0;
            }
        }
        /*
        else if(m_state == 3)
        {
            if(ui.bot_func == nullptr)
            {
                m_remapviewer.Initialize();
                m_remapviewer.HandOverUI();
                m_remapviewer.Controls();
                m_remapviewer.Finalize();
                m_state = 0;
            }
        }
        */
    }
}