
#include <frontend/sdl/sdl_window.h>

#ifdef GB_ENABLED
#include <frontend/sdl/gb_window.h>
#include "gb_window.cpp"
#endif

#ifdef GBA_ENABLED
#include <frontend/sdl/gba_window.h>
#include "gba_window.cpp"
#endif

#ifdef N64_ENABLED
#include <frontend/sdl/n64_window.h>
#include "n64_window.cpp"

#endif

#include <albion/destoer-emu.h>

void start_emu(std::string filename, Config& cfg)
{
	UNUSED(cfg);

	try
	{
		const auto type = get_emulator_type(filename);

        spdlog::info("Starting " + get_emulator_name(type) + " emulator engine.");

		switch(type)
		{
		#ifdef GB_ENABLED
			case emu_type::gameboy:
			{
				GameboyWindow gb;
				gb.main(filename,cfg.start_debug);
				break;
			}
		#endif

		#ifdef GBA_ENABLED
			case emu_type::gba:
			{
				GBAWindow gba;
				gba.main(filename,cfg.start_debug);
				break;
			}
		#endif

		#ifdef N64_ENABLED
			case emu_type::n64:
			{
				N64Window n64;
				n64.main(filename,cfg.start_debug);
				break;
			}
		#endif

			default:
			{
				spdlog::error("Unrecognised ROM type:", filename);
			}
		}
	}

	catch(std::exception &ex)
	{
		spdlog::error(ex.what());
		return;
	}
}


void SDLMainWindow::create_texture(u32 x, u32 y)
{
	// destroy the old one if need be
	if(texture)
	{
		SDL_DestroyTexture(texture);
	}

	X = x;
	Y = y;

	texture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, x, y);


	// resize the window
	SDL_SetWindowSize(window,x * 2, y * 2);
}

void SDLMainWindow::init_sdl(u32 x, u32 y)
{
	// initialize our window
	window = SDL_CreateWindow("albion",x * 2,y *2,SDL_WINDOW_RESIZABLE);
	
	// set a render for our window
	renderer = SDL_CreateRenderer(window, NULL);

	create_texture(x,y);
}

void SDLMainWindow::render(const u32* data)
{
    // do our screen blit
    SDL_UpdateTexture(texture, NULL, data,  4 * X);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);    	
}


SDLMainWindow::~SDLMainWindow()
{
	if(renderer)
	{
    	SDL_DestroyRenderer(renderer);
	}

	if(window)
	{
    	SDL_DestroyWindow(window);
	}

	if(texture)
	{
		SDL_DestroyTexture(texture);
	}

    SDL_Quit();  
}

bool window_in_focus(SDL_Window* window)
{
	return (SDL_GetWindowFlags(window) & SDL_WINDOW_OCCLUDED) != 0;
}

void SDLMainWindow::main(std::string filename, b32 start_debug)
{
	//constexpr uint32_t fps = 60; 
	//constexpr uint32_t screen_ticks_per_frame = 1000 / fps;
	//uint64_t next_time = current_time() + screen_ticks_per_frame;
	init(filename,playback);

	FpsCounter fps_counter;

	if(start_debug)
	{
		debug_halt();
	}

	playback.start();

	bool throttle = true;

    for(;;)
    {
		const auto start = std::chrono::steady_clock::now();

		fps_counter.reading_start();
		auto control = input.handle_input(window);
		
		pass_input_to_core();


		run_frame(window_in_focus(window));
		
		switch(control)
		{
			case emu_control::quit_t:
			{
				core_quit();
				break;
			}

			case emu_control::throttle_t:
			{
				//SDL_Delay(time_left(next_time));
				throttle = true;
				core_throttle();
				break;
			}

			case emu_control::unbound_t:
			{
				//SDL_Delay(time_left(next_time) / 8);
				throttle = false;
				core_unbound();
				break;
			}

			case emu_control::break_t:
			{
				debug_halt();
				break;
			}

			case emu_control::none_t: break;
		}

		fps_counter.reading_end();

		SDL_SetWindowTitle(window,fmt::format("albion: {:.2f}",fps_counter.get_fps()).c_str());


		const auto end = std::chrono::steady_clock::now();
		const s64 elapsed = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
		const s64 remain = ((1000'000'00 / 60) - elapsed) - overrun;


		if(throttle && remain > 0)
		{
			SDL_DelayPrecise(remain);
		}

		// we hit a breakpoint go back to the prompt
		handle_debug();
    }	
}
