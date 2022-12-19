
#pragma once
#include <3ds.h>

namespace LumaConfig{
    static inline void EnableGamePatching(){
        std::string data;
        FILE *f = fopen("/luma/config.ini", "r+");
        if(f)
        {
            fseek(f, 0L, SEEK_END);
            size_t size = ftell(f);
            data.resize(size);
            fseek(f, 0L, SEEK_SET);
            fread(&data[0], size, 1, f);
            const std::string s = "enable_game_patching = 1";
            auto found = data.find("enable_game_patching");
            if(found != std::string::npos)
                data.replace(found, s.length(), s);
            fseek(f, 0L, SEEK_SET);
            fwrite(&data[0], data.length(), 1, f);
            fclose(f);
        }
    }
};