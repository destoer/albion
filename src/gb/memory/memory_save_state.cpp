#include <gb/memory.h>

namespace gameboy
{

// TODO rebuild memory table on save state

// save states
void Memory::save_state(std::ofstream &fp)
{
    file_write_var(fp,hdma_len);
    file_write_var(fp,hdma_len_ticked);
    file_write_var(fp,dma_src);
    file_write_var(fp,dma_dst);
    file_write_var(fp,hdma_active);

    file_write_var(fp,enable_ram);
    file_write_var(fp,cart_ram_bank);
    file_write_var(fp,cart_rom_bank);
    file_write_var(fp,rom_banking);

    file_write_vec(fp,io);

    for(auto &x: vram)
    {
        file_write_vec(fp,x);
    }

    file_write_vec(fp,oam);
    file_write_vec(fp,wram);


    for(auto &x: cgb_wram_bank)
    {
        file_write_vec(fp,x);
    }


    for(auto &x: cart_ram_banks)
    {
        file_write_vec(fp,x);
    }

    file_write_var(fp,oam_dma_active);
    file_write_var(fp,oam_dma_address);
    file_write_var(fp,oam_dma_index);
    file_write_var(fp,cgb_wram_bank_idx);
    file_write_var(fp,vram_bank);
    file_write_var(fp,ignore_oam_bug);

    file_write_vec(fp,sgb_pal);
    file_write_vec(fp,sgb_packet);
    file_write_var(fp,sgb_transfer_active);
    file_write_var(fp,packet_count);
    file_write_var(fp,packet_len);
    file_write_var(fp,bit_count);

	// dont dump the memory table as its unecessary and unsafe
	// same goes for the rom and info struct

}

void Memory::load_state(std::ifstream &fp)
{
    file_read_var(fp,hdma_len);
    file_read_var(fp,hdma_len_ticked);
    file_read_var(fp,dma_src);
    file_read_var(fp,dma_dst);
    file_read_var(fp,hdma_active);

    file_read_var(fp,enable_ram);
    file_read_var(fp,cart_ram_bank);
    file_read_var(fp,cart_rom_bank);
    file_read_var(fp,rom_banking);

    file_read_vec(fp,io);

    for(auto &x: vram)
    {
        file_read_vec(fp,x);
    }

    file_read_vec(fp,oam);
    file_read_vec(fp,wram);


    for(auto &x: cgb_wram_bank)
    {
        file_read_vec(fp,x);
    }


    for(auto &x: cart_ram_banks)
    {
        file_read_vec(fp,x);
    }

    file_read_var(fp,oam_dma_active);
    file_read_var(fp,oam_dma_address);
    file_read_var(fp,oam_dma_index);
    file_read_var(fp,cgb_wram_bank_idx);
    file_read_var(fp,vram_bank);
    file_read_var(fp,ignore_oam_bug);

    file_read_vec(fp,sgb_pal);
    file_read_vec(fp,sgb_packet);
    file_read_var(fp,sgb_transfer_active);
    file_read_var(fp,packet_count);
    file_read_var(fp,packet_len);
    file_read_var(fp,bit_count);

    if(vram_bank > 1)
    {
        throw std::runtime_error("invalid vram bank");
    }

	// dont dump the memory table as its unecessary and unsafe
	// same goes for the rom and info struct

}

}