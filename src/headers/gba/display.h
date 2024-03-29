#pragma once
#include <albion/lib.h>
#include <gba/forward_def.h>
#include <gba/disp_io.h>

namespace gameboyadvance
{
static constexpr u32 SCREEN_WIDTH = 240;
static constexpr u32 SCREEN_HEIGHT = 160;

enum class display_mode
{
    visible,hblank,vblank
};

struct Display
{
    Display(GBA &gba);
    void init();
    void tick(int cycles);

    void update_vcount_compare();

    void insert_new_ppu_event(u32 next);


    void render_palette(u32 *palette, size_t size);
    void render_map(int id, std::vector<u32> &map);

    std::vector<u32> screen;
    bool new_vblank = false;
    DispIo disp_io;
    display_mode mode = display_mode::visible;

    struct TileData
    {
        TileData() {}

        TileData(u16 c, pixel_source s)
        {
            color = c;
            source = s;
        }

        u16 color = 0;
        pixel_source source = pixel_source::bd;
    };

    void render();
    void render_text(int id);
    void render_affine(int id);
    void advance_line();
    void render_sprites(int mode);
    void merge_layers();

    // is this inside a window if so is it enabled?
    bool bg_window_enabled(unsigned int bg, unsigned int x) const;

    // are sprites enabled inside a window
    bool sprite_window_enabled(unsigned int x) const;

    // are special effects enabled inside a window
    bool special_window_enabled(unsigned int x) const;

    void cache_window();
    bool is_bg_window_trivial(int id);

    // renderer helper functions
    u16 read_bg_palette(u32 pal_num,u32 idx);
    u16 read_obj_palette(u32 pal_num,u32 idx);

    void read_tile(TileData *tile,unsigned int bg,bool col_256,u32 base,u32 pal_num,u32 tile_num, 
        u32 y,bool x_flip, bool y_flip);
    
    void draw_tile(u32 x,const TileData &p);

    unsigned int cyc_cnt = 0; // current number of elapsed cycles
    unsigned int ly = 0; // current number of cycles
    
    bool window_0_y_triggered = false;
    bool window_1_y_triggered = false;

    Mem &mem;
    Cpu &cpu;
    GBAScheduler &scheduler;

    struct Scanline
    {
        TileData t1;
        TileData t2;
    };

    std::vector<Scanline> scanline;


    std::vector<TileData> sprite_line;
    std::vector<bool> sprite_semi_transparent;
    std::vector<window_source> window;
    std::vector<u32> oam_priority;
    std::vector<u32> sprite_priority;

};

u32 convert_color(u16 color);

}
