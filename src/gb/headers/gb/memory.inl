#pragma once



// read mem
template<bool DEBUG_ENABLE>
u8 Memory::read_mem(u16 addr) const noexcept
{
	const u8 value = read_mem_no_debug(addr);

	if constexpr(DEBUG_ENABLE)
	{
		if(debug.breakpoint_hit(addr,value,break_type::read))
		{
			// halt until told otherwhise :)
			debug.print_console("read breakpoint hit ({:x}:{:x})",addr,value);
			debug.halt();
		}
	}

	return value;
}


template<bool DEBUG_ENABLE>
void Memory::write_mem(u16 addr, u8 v) noexcept
{
	if constexpr(DEBUG_ENABLE)
	{
		if(debug.breakpoint_hit(addr,v,break_type::write))
		{
			// halt until told otherwhise :)
			debug.print_console("write breakpoint hit ({:x}:{:})",addr,v);
			debug.halt();
		}
	}

	write_mem_no_debug(addr,v);
}



template<bool DEBUG_ENABLE>
u16 Memory::read_word(u16 addr) noexcept
{
    return read_mem<DEBUG_ENABLE>(addr) | (read_mem<DEBUG_ENABLE>(addr+1) << 8);
}

template<bool DEBUG_ENABLE>
void Memory::write_word(u16 addr, u16 v) noexcept
{
    write_mem<DEBUG_ENABLE>(addr+1,(v&0xff00)>>8);
    write_mem<DEBUG_ENABLE>(addr,(v&0x00ff));
}

// maybe should have an eqiv for optimisation purposes where we know it cant trigger
template<bool DEBUG_ENABLE>
u8 Memory::read_memt_no_oam_bug(u16 addr) noexcept
{
	ignore_oam_bug = true;
	u8 v = read_mem<DEBUG_ENABLE>(addr);
	ignore_oam_bug = false;
	tick_access();
    return v;
}

// memory accesses (timed)
template<bool DEBUG_ENABLE>
u8 Memory::read_memt(u16 addr) noexcept
{
    u8 v = read_mem<DEBUG_ENABLE>(addr);
	tick_access();
    return v;
}

template<bool DEBUG_ENABLE>
void Memory::write_memt_no_oam_bug(u16 addr, u8 v) noexcept
{
	ignore_oam_bug = true;
    write_mem<DEBUG_ENABLE>(addr,v);
	ignore_oam_bug = false;
	tick_access();
}


template<bool DEBUG_ENABLE>
void Memory::write_memt(u16 addr, u8 v) noexcept
{
    write_mem<DEBUG_ENABLE>(addr,v);
	tick_access();
}


template<bool DEBUG_ENABLE>
u16 Memory::read_wordt(u16 addr) noexcept
{
    return read_memt<DEBUG_ENABLE>(addr) | (read_memt<DEBUG_ENABLE>(addr+1) << 8);
}

template<bool DEBUG_ENABLE>
void Memory::write_wordt(u16 addr, u16 v) noexcept
{
    write_memt<DEBUG_ENABLE>(addr+1,(v&0xff00)>>8);
    write_memt<DEBUG_ENABLE>(addr,(v&0x00ff));
}


template<bool DEBUG_ENABLE>
void Memory::do_dma(u8 v) noexcept
{
	scheduler.service_events();
	io[IO_DMA] = v; // write to the dma reg
	u16 dma_address = v << 8;
	// transfer is from 0xfe00 to 0xfea0
			
	/*// must be page aligned revisit later
	if(dma_address % 0x100) return;		
	*/

	// source must be below 0xe000
	// tick immediatly but keep the timing (not how hardware does it)
	// technically a oam dma should delay a cycle before writing
	if(dma_address < 0xe000)
	{
		oam_dma_disable();
		for(int i = 0; i < 0xA0; i++)
		{
			oam[i] = read_mem<DEBUG_ENABLE>(dma_address+i); 	
		}
		
		oam_dma_address = dma_address; // the source address
		oam_dma_index = 0; // how far along the dma transfer we are	

		const auto event = scheduler.create_event((0xa0 * 4)+0x8,gameboy_event::oam_dma_end);
		scheduler.insert(event,false);
	
		oam_dma_enable();
	}
}


template<bool DEBUG_ENABLE>
void Memory::do_gdma() noexcept
{
	const u16 source = dma_src & 0xfff0;
	
	const u16 dest = (dma_dst & 0x1ff0) | 0x8000;
	
	// hdma5 stores how many 16 byte incremnts we have to transfer
	const int len = ((io[IO_HDMA5] & 0x7f) + 1) * 0x10;

	// 8 t cycles to xfer one 0x10 block
	// (need to verify the timings on this!)

	for(int i = 0; i < len; i += 2)
	{
		write_mem<DEBUG_ENABLE>(dest+i,read_mem<DEBUG_ENABLE>(source+i));
		write_mem<DEBUG_ENABLE>(dest+i+1,read_mem<DEBUG_ENABLE>(source+i+1));
		tick_access();
	}

	io[IO_HDMA5] = 0xff; // terminate the transfer
}


template<bool DEBUG_ENABLE>
void Memory::do_hdma() noexcept
{

	if(!hdma_active)
	{
		return;
	}

	const u16 source = (dma_src & 0xfff0) + hdma_len_ticked*0x10;

	const u16 dest = ((dma_dst & 0x1ff0) | 0x8000) + hdma_len_ticked*0x10;

	/*if(!(source <= 0x7ff0 || ( source >= 0xa000 && source <= 0xdff0)))
	{
		printf("ILEGGAL HDMA SOURCE: %X!\n",source);
		exit(1);
	}
	*/

	// 8 t cycles to xfer one 0x10 block

	// find out how many cycles we tick but for now just copy the whole damb thing 						
	for(int i = 0; i < 0x10; i += 2)
	{
		write_mem<DEBUG_ENABLE>(dest+i,read_mem<DEBUG_ENABLE>(source+i));
		write_mem<DEBUG_ENABLE>(dest+i+1,read_mem<DEBUG_ENABLE>(source+i+1));
		tick_access();
	}
	
	// hdma is over 
	if(--hdma_len <= 0)
	{
		// indicate the tranfser is over
		io[IO_HDMA5] = 0xff;
		hdma_active = false;
	}

	// goto next block
	else
	{
		hdma_len_ticked++;
	}	
}

template<bool DEBUG_ENABLE>
void Memory::write_iot(u16 addr, u8 v) noexcept
{
	if constexpr(DEBUG_ENABLE)
	{
		if(debug.breakpoint_hit(addr,v,break_type::write))
		{
			// halt until told otherwhise :)
			debug.print_console("write breakpoint hit ({:x}:{:})",addr,v);
			debug.halt();
		}
	}

	scheduler.service_events();
    write_io<DEBUG_ENABLE>(addr,v);
	tick_access();
}


// high ram 0xf000
// we bundle io into this but the hram section is at 0xff80-ffff
template<bool DEBUG_ENABLE>
void Memory::write_hram(u16 addr,u8 v) noexcept
{
	scheduler.service_events();
    // io regs
    if(addr >= 0xff00)
    {
        write_io<DEBUG_ENABLE>(addr,v);
    }

    // high wram mirror
    else if(addr >= 0xf000 && addr <= 0xfdff)
    {
        std::invoke(memory_table[0xd].write_memf,this,addr,v);
    }

	// oam is accesible during mode 0-1
	else if(addr >= 0xfe00 && addr <= 0xfeff)
	{
		write_oam(addr,v);
		return;
	}


    else // restricted
    {

    }
}


template<bool DEBUG_ENABLE>
u8 Memory::read_iot(u16 addr) noexcept
{
	scheduler.service_events();
    const u8 value = read_io(addr);
	tick_access();

	if constexpr(DEBUG_ENABLE)
	{
		if(debug.breakpoint_hit(addr,value,break_type::read))
		{
			// halt until told otherwhise :)
			debug.print_console("read breakpoint hit ({:x}:{:x})",addr,value);
			debug.halt();
		}
	}
	return value;	
}
