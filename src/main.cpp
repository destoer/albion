#include <frontend/sdl/sdl_window.h>
#include <albion/lib.h>
#include <iostream>

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL3/SDL.h>
#endif

#include "test.cpp"
#include "spdlog/spdlog.h"
#include <cfenv>

int main(int argc, char *argv[])
{  
    UNUSED(argc); UNUSED(argv);
  
    if(argc == 2)
    {
        std::string arg(argv[1]);
        if(arg == "-t")
        {
            try
            {
                run_tests();
            }

            catch(std::exception &ex)
            {
                std::cout << ex.what();
            }

            return 0;
        }
    }

    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [%l] %v");
    std::fesetround(FE_TONEAREST);

	if(!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_VIDEO))
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

    Config cfg = get_config(argc,argv);

    if(argc < 2)
    {
        printf("usage: %s <rom_name>\n",argv[0]);
        return 0;
    }
    start_emu(argv[1],cfg);

    SDL_Quit();

    return 0;
}
