#include <gb/ppu.h>

namespace gameboy
{

// save states
void Ppu::save_state(std::ofstream &fp)
{
    file_write_vec(fp,screen);
    file_write_var(fp,current_line);
    file_write_var(fp,mode);
    file_write_var(fp,new_vblank);
    file_write_var(fp,signal);
    file_write_var(fp,scanline_counter);
    file_write_var(fp,x_cord);
    file_write_var(fp,bg_fifo);
    file_write_var(fp,obj_fifo);
    file_write_var(fp,fetcher);
    file_write_var(fp,objects);
    file_write_var(fp,tile_cord);
    file_write_var(fp,no_sprites);
    file_write_var(fp,cur_sprite);
    file_write_var(fp,scx_cnt);
    file_write_arr(fp,bg_pal,sizeof(bg_pal));
    file_write_arr(fp,sp_pal,sizeof(sp_pal));
    file_write_var(fp,sp_pal_idx);
    file_write_var(fp,bg_pal_idx);
    file_write_var(fp,window_y_line);
    file_write_var(fp,window_x_line);
    file_write_var(fp,window_x_triggered);
    file_write_var(fp,window_y_triggered);
    file_write_var(fp,pixel_transfer_end);
    file_write_var(fp,emulate_pixel_fifo);
    file_write_var(fp,early_line_zero);
    file_write_var(fp,glitched_oam_mode);
    file_write_var(fp,mask_en);
    file_write_arr(fp,dmg_pal,sizeof(dmg_pal));
}

dtr_res Ppu::load_state(std::ifstream &fp)
{
    dtr_res err = file_read_vec(fp,screen);
    err |= file_read_var(fp,current_line);
    if(current_line > 153)
    {
        spdlog::error("current line out of range!");
        return dtr_res::err;
    }

    err |= file_read_var(fp,mode);
    if(static_cast<int>(mode) > 3 || static_cast<int>(mode) < 0)
    {
        spdlog::error("load_state invalid ppu mode!");
        return dtr_res::err;
    }
    err |= file_read_var(fp,new_vblank);
    err |= file_read_var(fp,signal);
    err |= file_read_var(fp,scanline_counter);
    err |= file_read_var(fp,x_cord);

    err |= file_read_var(fp,bg_fifo);
    err |= file_read_var(fp,obj_fifo);
    err |= file_read_var(fp,fetcher);
    err |= file_read_var(fp,objects);

    if(fetcher.len > 8)
    {
        spdlog::error("invalid fecther len");
        return dtr_res::err;
    }


    if(bg_fifo.read_idx >= bg_fifo.size || bg_fifo.write_idx >= bg_fifo.size || bg_fifo.len > bg_fifo.size)
    {
        spdlog::error("invalid bg fifo");
        return dtr_res::err;
    } 


    if(obj_fifo.read_idx >= obj_fifo.size || obj_fifo.write_idx >= obj_fifo.size || obj_fifo.len > obj_fifo.size)
    {
        spdlog::error("invalid sp fifo");
        return dtr_res::err;
    } 

    if(fetcher.len > 8)
    {
        spdlog::error("invalid fetcher len");
        return dtr_res::err;
    }

    for(size_t i = 0; i < obj_fifo.len; i++)
    {
        size_t fifo_idx = (obj_fifo.read_idx + i) % obj_fifo.len;
        if(obj_fifo.fifo[fifo_idx].sprite_idx >= no_sprites)
        {
            spdlog::error("inavlid sprite index in fifo");
            return dtr_res::err;
        }
    }


    err |= file_read_var(fp,tile_cord);
    if(tile_cord >= 255) // can fetch past the screen width
    {
        spdlog::error("tile cord out of range!");
        return dtr_res::err;
    }

    err |= file_read_var(fp,no_sprites);
    err |= file_read_var(fp,cur_sprite);
    if(no_sprites > 10)
    {
        spdlog::error("invalid number of sprites!");
        return dtr_res::err;
    }

    if(cur_sprite > no_sprites)
    {
        spdlog::error("invalid current sprite");
        return dtr_res::err;
    }
    err |= file_read_var(fp,scx_cnt);
    err |= file_read_arr(fp,bg_pal,sizeof(bg_pal));
    err |= file_read_arr(fp,sp_pal,sizeof(sp_pal));
    err |= file_read_var(fp,sp_pal_idx);
    err |= file_read_var(fp,bg_pal_idx);
    err |= file_read_var(fp,window_y_line);
    err |= file_read_var(fp,window_x_line);
    err |= file_read_var(fp,window_x_triggered);
    err |= file_read_var(fp,window_y_triggered);
    err |= file_read_var(fp,pixel_transfer_end);
    err |= file_read_var(fp,emulate_pixel_fifo);
    err |= file_read_var(fp,early_line_zero);
    err |= file_read_var(fp,glitched_oam_mode);
    err |= file_read_var(fp,mask_en);
    err |= file_read_arr(fp,dmg_pal,sizeof(dmg_pal));

    return err;
}

}