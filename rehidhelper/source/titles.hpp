#pragma once
#include <vector>
#include <3ds.h>
#include <citro2d.h>
#include <string>
#include "smdh.hpp"
class Titles
{
    public:
        void PopulateTitleArray();
        void PopulateSMDHArray();
        void ConvertSMDHsToC2D();
        void ConvertDescsToC2DText();
        void FilterOutTWLAndUpdate();
        void AddGlobalEntry();
        const std::vector<u64> GetTitles() { return m_oktitlesvector; };
        u32 GetCount() { return m_oktitlesvector.size(); };
        const std::vector<C2D_Image> GetC2DSMDHImgs() { return m_images; };
        std::vector<C2D_Text> GetC2DDescText() { return m_c2ddescvector; };
        std::vector<std::string> GetTitlesDescription() { return m_descvector; };
    private:
        uint64_t *m_array;
        uint32_t m_count;
        std::vector<uint64_t> m_filteredvector;
        std::vector<uint64_t> m_oktitlesvector;
        std::vector<std::vector<uint8_t>> m_smdhvector;
        std::vector<std::string> m_descvector;
        std::vector<C2D_Image> m_images;
        std::vector<C3D_Tex *> m_texs;
        std::vector<C2D_Text> m_c2ddescvector;
        uint64_t m_gamecardid;
        C2D_SpriteSheet m_sheet;
        C2D_Image m_globalimg;
};