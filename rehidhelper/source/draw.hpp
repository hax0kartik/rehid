#include <string>
#include <vector>
#include <3ds.h>
#include <citro2d.h>
#include "ui.hpp"

namespace Draw 
{
    static float ctr = 0.0f;
    void DrawBars(bool top, float thickness);
    void DrawTextInCentre(bool top, LightLock *lock, std::string *str);
    void Circle(float center_x, float center_y, float radius);
    void DrawLoadingBarAndText(LightLock *lock, std::string *str);
    void DrawGameSelectionScreen(LightLock *lock, const std::vector<C2D_Image> &imgs, int *selected, int *page);
    void DrawMainMenuTop(LightLock *lock, C2D_Image image, int *selected, const std::vector<std::string> &descs);
    void DrawMainMenu(LightLock *lock, int *selected, const std::vector<std::string> &options);
    void DrawLoadingScreen(LightLock *lock);
    void DrawConfigScreen(const std::string &data);
    void DrawTitleInfo(LightLock *lock, const std::vector<std::string> &descs, int *selected);
};