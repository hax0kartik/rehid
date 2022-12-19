#pragma once
#include <3ds.h>
#include <type_traits>

constexpr static void inline ExclusiveWrite(auto *addr, auto val){
    if constexpr(std::is_same_v<decltype(val), s8> || std::is_same_v<decltype(val), u8>){
        do
            __ldrexb(addr);
        while ( __strexb(addr, val));
    } else if constexpr(std::is_same_v<decltype(val), s16> || std::is_same_v<decltype(val), u16>){
        do
            __ldrexh(addr);
        while ( __strexh(addr, val));
    } else if constexpr(std::is_same_v<decltype(val), s32> || std::is_same_v<decltype(val), u32>){
        do
            __ldrex(addr);
        while ( __strex(addr, val));
    } else {
       // #error "Unknown Type"
    }
}