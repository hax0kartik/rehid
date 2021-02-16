#pragma once
#include <3ds.h>

struct KeyState
{
    uint32_t oldkey; // Old Key Combo
    uint32_t newkey; // New Key Combo
};

struct key_s
{
    char key[10];
    uint32_t val;
};

class Remapper
{
    public:
        void GenerateFileLocation();
        uint32_t Remap(uint32_t hidtstate);
        Result ReadConfigFile();
        void ParseConfigFile();
    private:
        KeyState m_remapstates[10]; // Support upto 10 remapable combos
        uint8_t m_entries;
        char *m_filedata;
        uint64_t m_filedatasize;
        char m_fileloc[40];
};