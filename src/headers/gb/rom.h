#pragma once
#include <albion/lib.h>

namespace gameboy
{

enum class rom_type
{
    mbc1,mbc2,mbc3,mbc5,rom_only
};

struct RomInfo
{
    void init(std::vector<u8> &rom, std::string romname);

    unsigned int no_ram_banks = 0;
    unsigned int no_rom_banks = 0;
    int cart_type = -1;
    std::string filename; 
    rom_type type;
    bool has_rtc = false;
};

}