#include <gb/gb.h>
#include <albion/input.h>

namespace gameboy
{

GB::GB()
{
	// setup a dummy state
	reset("no rom",false,false);
	// default the breakpoints to off
	// maybe something to consider as a config
	change_breakpoint_enable(false);
}

void GB::reset(std::string rom_name, bool with_rom, bool use_bios)
{
	//use_bios = true;
	scheduler.init();
    mem.init(rom_name,with_rom,use_bios);
	cpu.init(use_bios);
    ppu.init();
    disass.init();
	const auto mode = cpu.is_cgb? gameboy_psg::psg_mode::cgb : gameboy_psg::psg_mode::dmg;

	apu.init(mode,use_bios);
	throttle_emu = true;
	if(use_bios)
	{
		mem.bios_enable();
	}
	//printf("cgb: %s\n",cpu.is_cgb? "true" : "false");
}


void GB::change_breakpoint_enable(bool enabled)
{
	debug.breakpoints_enabled = enabled;

	mem.change_breakpoint_enable(enabled);
}

// need to do alot more integrity checking on data in these :)
void GB::save_state(std::string filename)
{
	std::cout << "save state: " << filename << "\n";


	std::ofstream fp(filename,std::ios::binary);
	if(!fp)
	{
		cpu.panic("Could not save state to file");
	}


	cpu.save_state(fp);
	mem.save_state(fp);
	ppu.save_state(fp);
	apu.save_state(fp);
	scheduler.save_state(fp);

	fp.close();
}


void GB::load_state(std::string filename)
{
	std::cout << "load state: " << filename << "\n";

	dtr_res err = dtr_res::ok;

	std::ifstream fp(filename,std::ios::binary);
	if(!fp)
	{
		cpu.panic("could not open save state file");
		return;
	}

	err |= cpu.load_state(fp);
	err |= mem.load_state(fp);
	err |= ppu.load_state(fp);
	err |= apu.load_state(fp);
	err |= scheduler.load_state(fp);

	fp.close();

	if(!err)
	{
		cpu.panic("Could not load state");
	}
}

void GB::handle_input(Controller& controller)
{
	for(auto& event : controller.input_events)
	{
		switch(event.input)
		{
			case controller_input::start: key_input(button::start,event.down); break;
			case controller_input::select: key_input(button::select,event.down); break;
			case controller_input::a: key_input(button::a,event.down); break;
			case controller_input::x: key_input(button::b,event.down); break;

			case controller_input::up: key_input(button::up,event.down); break;
			case controller_input::down: key_input(button::down,event.down); break;
			case controller_input::left: key_input(button::left,event.down); break;
			case controller_input::right: key_input(button::right,event.down); break;

			default: break;
		}
	}
}

void GB::key_input(button b, b32 down)
{
	if(down)
	{
		key_pressed(b);
	}

	else
	{
		key_released(b);
	}
}

// our "frontend" will call these
void GB::key_released(button b)
{
	auto key = static_cast<int>(b); 
	cpu.joypad_state = set_bit(cpu.joypad_state, key);
}


void GB::key_pressed(button b)
{
	const auto key = static_cast<int>(b);

	// if setting from 1 to 0 we may have to req 
	// and interrupt
	const bool previously_unset = is_set(cpu.joypad_state,key);
	
	// remember if a key is pressed its bit is 0 not 1
	cpu.joypad_state = deset_bit(cpu.joypad_state, key);
	
	// is the input for a button?
	const bool button = (key > 3);

	
	const auto key_req = mem.io[IO_JOYPAD];
	bool req_int = false;
	
	// only fire interrupts on buttons the game is current trying to read
	// a, b, start, sel
	if(button && !is_set(key_req,5))
	{
		req_int = true;
	}
	
	// dpad
	else if(!button && !is_set(key_req,4))
	{
		req_int = true;
	}
	
	if(req_int && previously_unset)
	{
		cpu.request_interrupt(4);
	}
}

// run a frame
void GB::run()
{
	if(cpu.panicked)
	{
		return;
	}

    ppu.new_vblank = false;
	cpu.cycle_frame = false;
	cpu.insert_new_cycle_event();

	if(debug.is_halted())
	{
		return;
	}

	if(debug.breakpoints_enabled)
	{
		// break out early if we have hit a debug event
		while(!cpu.cycle_frame) 
		{
			cpu.exec_instr<true>();
			if(debug.is_halted())
			{
				return;
			}
		}
	}

	else
	{
		while(!cpu.cycle_frame) 
		{
			cpu.exec_instr<false>();
		}
	}

	if(throttle_emu)
	{
		mem.frame_end();
	}
}

}