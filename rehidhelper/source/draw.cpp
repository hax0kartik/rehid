#include "draw.hpp"

void Draw::DrawBars(bool top, float thickness)
{
    float w, h = 240.0f;
    w = top ? 400.0f : 320.0f;
    C2D_DrawRectSolid (0.0f, 0.0f, 0.0f, w, thickness, 0xff227ee6);
    C2D_DrawRectSolid (0.0f, h - thickness, 0.0f, w, thickness, 0xff227ee6);
}

void Draw::DrawTextInCentre(bool top, LightLock *lock, std::string *str)
{
    C2D_Text txt;
    C2D_TextBuf buf = top == true ? ui.top_text_buf : ui.bot_text_buf;
    LightLock_Lock(lock);
    C2D_TextBufClear(buf);
    DrawBars(top, 5.0f);
    C2D_TextParse(&txt, buf, str->c_str());
    C2D_TextOptimize(&txt);
    float outwidth, outheight;
    C2D_TextGetDimensions (&txt, 1.0f, 1.0f, &outwidth , &outheight);
    C2D_DrawText(&txt, C2D_AlignCenter, top == true ? 200.0f : 160.0f, (240.0f - outheight) / 2, 1.0f, .5f, .5f);
    LightLock_Unlock(lock);
}

void Draw::Circle(float center_x, float center_y, float radius)
{
    float point_x = center_x + cos(ctr)*radius;
    float point_y = center_y + sin(ctr)*radius;
    C2D_DrawCircleSolid (point_x, point_y, 1.0f, 1.0f, 0xff000000);
    ctr += 0.1f;
}

void Draw::DrawLoadingBarAndText(LightLock *lock, std::string *str)
{
    C2D_Text txt;
    C2D_TextBuf buf = ui.bot_text_buf;
    LightLock_Lock(lock);
    DrawBars(false, 5.0f);
    C2D_TextBufClear(buf);
    C2D_TextParse(&txt, buf, str->c_str());
    C2D_TextOptimize(&txt);
    float outwidth, outheight;
    C2D_TextGetDimensions (&txt, 1.0f, 1.0f, &outwidth , &outheight);
    C2D_DrawText(&txt, C2D_AlignCenter, 160.0f, 120.0f - (outheight/2), 1.0f, 0.5f, 0.5f);
    float circlex, circley;
    float R = 5.0f;
    circlex = 160.0 - R;
    circley = (120.0 - outheight/2) + 45.0 - R;
    Circle(circlex, circley, 5.0f);
    LightLock_Unlock(lock);
}

int NO_OF_ICONS_PER_PAGE = 10 * 7;
void Draw::DrawGameSelectionScreen(LightLock *lock, const std::vector<C2D_Image> &imgs, int *selected, int *page)
{
    LightLock_Lock(lock);
    DrawBars(false, 5.0f);
    float x = 13.0f, y = 15.0f;
    for(int i = 0; i < NO_OF_ICONS_PER_PAGE && (*page * NO_OF_ICONS_PER_PAGE) + i < imgs.size(); i++)
    {
        if(x + imgs[(*page * NO_OF_ICONS_PER_PAGE) + i].subtex->width > 320.0f)
        {
            y = y + imgs[(*page * NO_OF_ICONS_PER_PAGE) + i].subtex->height + 6.0f;
            x = 13.0f;
        }
        C2D_ImageTint tint;
        if(((*page * NO_OF_ICONS_PER_PAGE) + i) == *selected)
            C2D_AlphaImageTint(&tint, 1.0f);
        else
            C2D_AlphaImageTint(&tint, 0.5f);
        C2D_DrawImageAt(imgs[(*page * NO_OF_ICONS_PER_PAGE) + i], x, y, 1.0f, &tint);
        x += imgs[(*page * NO_OF_ICONS_PER_PAGE) + i].subtex->width + 6.0f;
    }
    LightLock_Unlock(lock);
}

void Draw::DrawMainMenuTop(LightLock *lock, C2D_Image image, int *selected, const std::vector<std::string> &descs)
{
    LightLock_Lock(lock);
    DrawBars(true, 5.0f);
    C2D_Text txt[2];
    const char *str = "Rehid Helper.";
    C2D_TextBuf buf = ui.top_text_buf;
    C2D_TextBufClear(buf);
    C2D_TextParse(&txt[0], buf, descs[*selected].c_str());
    C2D_TextParse(&txt[1], buf, str);
    C2D_TextOptimize(&txt[0]);
    C2D_TextOptimize(&txt[1]);
    float outwidth, outheight;
    C2D_TextGetDimensions (&txt[1], 1.0f, 1.0f, &outwidth ,&outheight);
    C2D_DrawText(&txt[0], 0, 10.0f, 210.0f, 1.0f, 0.5f, 0.5f);
    C2D_DrawText(&txt[1], C2D_AlignCenter, 200.0f, (240.0f - outheight) / 2, 1.0f, 1.0f, 1.0f);
    LightLock_Unlock(lock);
}

void Draw::DrawMainMenu(LightLock *lock, int *selected, const std::vector<std::string> &options)
{
    LightLock_Lock(lock);
    DrawBars(false, 5.0f);
    C2D_Text txt[3];
    C2D_TextBuf buf = ui.bot_text_buf;
    C2D_TextBufClear(buf);
    float x = 160.0f; float y = 10.0f;
    float outwidth, outheight = 30.0f;
    y = (240.0f - (options.size() * outheight)) / 2;
    for(int i = 0; i < options.size(); i++)
    {
        C2D_TextParse(&txt[i], buf, options[i].c_str());
        C2D_TextOptimize(&txt[i]);
        C2D_TextGetDimensions (&txt[i], 1.0f, 1.0f, &outwidth ,&outheight);
        if(i == *selected)
            C2D_DrawRectSolid (10.0f, y - 5.0f, 0.5f, 300.0f, outheight, 0x50227ee6);
        C2D_DrawText(&txt[i], C2D_AlignCenter, x, y , 1.0f, 0.5f, 0.5f);
        y += outheight;
    }
    LightLock_Unlock(lock);
}

void Draw::DrawLoadingScreen(LightLock *lock)
{
    LightLock_Lock(lock);
    DrawBars(true, 5.0f);
    C2D_Text txt;
    const char *str = "Rehid Helper.";
    C2D_TextBuf buf = ui.top_text_buf;
    C2D_TextBufClear(buf);
    C2D_TextParse(&txt, buf, str);
    C2D_TextOptimize(&txt);
    float outwidth, outheight;
    C2D_TextGetDimensions (&txt, 1.0f, 1.0f, &outwidth ,&outheight);
    C2D_DrawText(&txt, C2D_AlignCenter, 200.0f, (240.0f - outheight) / 2, 1.0f, 1.0f, 1.0f);
    float circlex, circley;
    float R = 10.0f;
    circlex = 200.0 - R;
    circley = (120.0 - outheight/2) + 80.0 - R;
    Circle(circlex, circley, R);
    LightLock_Unlock(lock);
}

void Draw::DrawConfigScreen(const std::string &data)
{
    DrawBars(true, 5.0f);
    C2D_Text txt;
    C2D_TextBuf buf = ui.top_text_buf;
    C2D_TextBufClear(buf);
    C2D_TextParse(&txt, buf, data.c_str());
    C2D_TextOptimize(&txt);
    C2D_DrawText(&txt, 0, 10.0f, 10.0f, 0.5f, 0.5f, 0.5f);
}

void Draw::DrawTitleInfo(LightLock *lock, const std::vector<std::string> &descs, int *selected)
{
    LightLock_Lock(lock);
    DrawBars(true, 5.0f);
    const char *str = "Current Title: ";
    const char *str2 = "Rehid Helper.";
    C2D_Text txt[3];
    C2D_TextBuf buf = ui.top_text_buf;
    C2D_TextBufClear(buf);
    C2D_TextParse(&txt[0], buf, str);
    C2D_TextParse(&txt[1], buf, descs[*selected].c_str());
    C2D_TextParse(&txt[2], buf, str2);
    float outwidth, outheight;
    C2D_TextGetDimensions (&txt[0], 1.0f, 1.0f, &outwidth ,&outheight);
    C2D_TextOptimize(&txt[0]);
    C2D_TextOptimize(&txt[1]);
    C2D_TextOptimize(&txt[2]);
    C2D_DrawText(&txt[0], 0, 10.0f, 210.0f, 1.0f, 0.5f, 0.5f);
    C2D_DrawText(&txt[1], 0, 10.0f + (outwidth/2), 210.0f, 1.0f, 0.5f, 0.5f);
    C2D_TextGetDimensions (&txt[2], 1.0f, 1.0f, &outwidth ,&outheight);
    C2D_DrawText(&txt[2], C2D_AlignCenter, 200.0f, (240.0f - outheight) / 2, 1.0f, 1.0f, 1.0f);
    LightLock_Unlock(lock);
}