#include <gba/gba.h>

namespace gameboyadvance
{

void Display::render_sprites(int mode)
{
    const TileData lose_bg(read_bg_palette(0,0),pixel_source::bd);
    // make all of the line lose
    // until something is rendred over it
    std::fill(sprite_line.begin(),sprite_line.end(),lose_bg);
    std::fill(sprite_semi_transparent.begin(),sprite_semi_transparent.end(),false);
    std::fill(sprite_priority.begin(),sprite_priority.end(),5);

    // objects aernt enabled do nothing more
    if(!disp_io.disp_cnt.obj_enable)
    {
        return;
    }


    std::fill(oam_priority.begin(),oam_priority.end(),128+1);

    const bool is_bitmap = mode >= 3;

    // have to traverse it in forward order
    // even though reverse is easier to handle most cases
    for(u32 i = 0; i < 128; i++)
    {
        int obj_idx = i * 8;
        

        const auto attr0 = handle_read<u16>(mem.oam,obj_idx);
        const auto attr1 = handle_read<u16>(mem.oam,obj_idx+2);
        const auto attr2 = handle_read<u16>(mem.oam,obj_idx+4);

        const bool affine = is_set(attr0,8);

        // should check mosaic by here but we will just ignore it for now

        // disable bit in regular mode
        if(is_set(attr0,9) && !affine)
        {
            continue;
        }

        const int obj_mode = (attr0 >> 10) & 0x3;


        // prohibited is this ignored on hardware
        // or does it behave like another?
        if(obj_mode == 3)
        {
            continue;
        }

        const int shape = (attr0 >> 14) & 0x3;

        // prohibited is this ignored on hardware
        // or does it behave like another?
        if(shape == 3)
        {
            continue;
        }


        const int obj_size = (attr1 >> 14) & 0x3;

        static constexpr int32_t x_size_lookup[3][4] = 
        {
            {8,16,32,64},
            {16,32,32,64},
            {8,8,16,32}
        };

        static constexpr int32_t y_size_lookup[3][4] = 
        {
            {8,16,32,64},
            {8,8,16,32},
            {16,32,32,64}
        };

        auto y_size = y_size_lookup[shape][obj_size];
        auto x_size = x_size_lookup[shape][obj_size];

        // original size of the sprite that is not affected by the double size flag
        const auto x_sprite_size = x_size;
        const auto y_sprite_size = y_size;
        const bool double_size = is_set(attr0,9) && affine;

        u32 y_cord = attr0 & 0xff;

        // current x cords greater than screen width are handled in the decode loop
        // by ignoring them until they are in range
        u32 x_cord = attr1 & 511;

        // bounding box even if double isnt going to draw outside 
        // because of how we operate on it
        // how to get this working?


        // on the top and left side its not going to extend
        // only to the postive so we need to find a way to "centre" it
        // see tonc graphical artifacts
        if(double_size)
        {
            x_size *= 2;
            y_size *= 2;
        }



        // if cordinate out of screen bounds and does not wrap around
        // then we dont care
        if(x_cord >= SCREEN_WIDTH && x_cord + x_size < 512)
        {
            continue;
        }


        // check we intersect with the current ly
        bool line_overlap;

        if(y_cord < SCREEN_HEIGHT)
        {
            line_overlap = y_cord + y_size > ly && y_cord <= ly;
        }

        // overflowed from 255
        else
        {
            // by definiton it is allways greater than ly before it overflows
            u8 y_end = (y_cord + y_size) & 0xff;
            line_overlap = y_end >= ly && y_end < SCREEN_HEIGHT; 
        }

        if(!line_overlap)
        {
            continue;   
        }

        const bool color = is_set(attr0,13);

        // assume palette
        unsigned int tile_num = attr2 & 0x3ff;
        // lower bit ignored in 2d mapping
        if(color && !disp_io.disp_cnt.obj_vram_mapping)
        {
            tile_num &= ~1;
        }
        const unsigned int pal =  (attr2 >> 12) & 0xf;
        const unsigned int priority = (attr2 >> 10) & 3;


        // bitmap modes starts at  0x14000 instead of 0x10000
        // because of bg map tiles below this are simply ignored
        if(is_bitmap && tile_num < 512)
        {
            continue;
        }


        // merge both into a single loop by here and dont worry about it being fast
        // or this will fast become a painful mess to work with
        // figure out how the affine transforms is actually calculated
        const bool x_flip = is_set(attr1,12) && !affine;
        const bool y_flip = is_set(attr1,13) && !affine;


        const u32 aff_param = (attr1 >> 9) & 31;

        // rotation centre
        const int32_t x0 = x_sprite_size / 2;
        const int32_t y0 = y_sprite_size / 2; 


        const int32_t y_max = y_size - 1;
        const int32_t y1 = y_flip?  y_max - ((ly-y_cord) & y_max) : ((ly-y_cord) & y_max);

        for(int32_t x1 = 0; x1 < x_size; x1++)
        {


            const u32 x_offset = (x_cord + x1) & 511;

            // probably a nicer way to do this but this is fine for now
            if(x_offset >= SCREEN_WIDTH)
            {
                continue;
            }


            int32_t y2 = y1;
            int32_t x2 = x1;

            if(affine)
            {
                const auto base = aff_param*0x20;

                // 8.8 fixed point
                const int16_t pa = handle_read<u16>(mem.oam,base+0x6);
                const int16_t pb = handle_read<u16>(mem.oam,base+0xe);
                const int16_t pc = handle_read<u16>(mem.oam,base+0x16);
                const int16_t pd = handle_read<u16>(mem.oam,base+0x1e);


                
                const int32_t x_param = x1 - (x_size / 2);
                const int32_t y_param = y1 - (y_size / 2);

                // perform the affine transform (8.8 fixed point)
                x2 = ((pa*x_param + pb*y_param) >> 8) + x0;
                y2 = ((pc*x_param + pd*y_param) >> 8) + y0;

                // out of range transform pixel is transparent
                if(x2 >= x_sprite_size || y2 >= y_sprite_size || x2 < 0 || y2 < 0)
                {
                    continue;
                }
            }


            else if(x_flip)
            {
                x2 = x_size - x2 - 1;
            }


            u32 tile_offset;

            // 1d object mapping
            if(disp_io.disp_cnt.obj_vram_mapping)
            {
               tile_offset =  ((y2 / 8) * (x_sprite_size / 8)) + (x2 / 8);
            }

            // 2d object mapping
            // in 4bpp 1024 tiles split into 32 by 32
            // or 16 by 32 in 8bpp mode
            else
            {
                tile_offset = ((y2 / 8) * (32 >> color)) + (x2 / 8);
            }

            const auto &disp_cnt = disp_io.disp_cnt;

            // 4bpp
            if(!color)
            {

                // base + tile_base * tile_size
                const u32 addr = 0x10000 + ((tile_offset + tile_num) * 8 * 4);

                const u32 data_offset = ((x2 % 8) / 2) + ((y2 % 8) * 4);
                const auto tile_data = mem.vram[addr+data_offset];

                // lower x cord stored in lower nibble
                const u32 idx = ((x2 & 1)? (tile_data >> 4) : tile_data) & 0xf;

                // object window obj not displayed any non zero pixels are 
                // the object window
                if(idx != 0 && obj_mode == 2 && disp_cnt.obj_window_enable)
                {
                    // window 0 and 1 have higher priority
                    if(window[x_offset] == window_source::out)
                    {
                        window[x_offset] = window_source::obj;
                    }
                }    

                else
                {
                    if(i < oam_priority[x_offset])
                    {
                        if(idx != 0)
                        {
                            const auto color = read_obj_palette(pal,idx);
                            sprite_line[x_offset].color = color;
                            sprite_line[x_offset].source = pixel_source::obj;
                            oam_priority[x_offset] = i;
                        }

                         // hardware bug priority is updated even if transparent
                        sprite_priority[x_offset] = priority;
                    }
                }

            }


            else
            {

                // base + tile_base * tile_size
                // tile size is still 32 bytes for the tile num in 256 for some reason (thanks fleroviux)
                // even though the bg uses the logical 64...
                // the actual offset into it because of the cords is still 64
                const u32 addr = 0x10000 + (tile_num * 8 * 4) + (tile_offset * 8 * 8);

                const u32 data_offset = (x2 % 8) + ((y2 % 8) * 8);
                const auto tile_data = mem.vram[addr+data_offset];

                // object window obj not displayed any non zero pixels are 
                // the object window
                if(tile_data != 0 && obj_mode == 2 && disp_cnt.obj_window_enable)
                {
                    // window 0 and 1 have higher priority
                    if(window[x_offset] == window_source::out)
                    {
                        window[x_offset] = window_source::obj;
                    }
                }    

                else
                {
                    if(i < oam_priority[x_offset])
                    {
                        if(tile_data != 0)
                        {
                            const auto color = read_obj_palette(0,tile_data);
                            sprite_line[x_offset].color = color;
                            sprite_line[x_offset].source = pixel_source::obj;
                            oam_priority[x_offset] = i;
                        }

                        // hardware bug priority is updated even if transparent
                        sprite_priority[x_offset] = priority;
                    }
                }


            }

            if(obj_mode == 1)
            {
                sprite_semi_transparent[x_offset] = true;
            }
        }                

    }
}

}