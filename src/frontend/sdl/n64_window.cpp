#include "n64_window.h"


void N64Window::init(const std::string& filename,Playback& playback)
{
    UNUSED(playback);
    init_sdl(320,240);
    input.init();
    reset(n64,filename);
    input.controller.simulate_dpad = false;	
    playback.init(n64.audio_buffer);
}

void N64Window::pass_input_to_core()
{
    nintendo64::handle_input(n64,input.controller);
    input.controller.input_events.clear();
}

void N64Window::core_quit()
{
    //n64.mem.save_cart_ram();
    exit(0);   
}

void N64Window::run_frame(bool paused)
{
    if(!paused)
    {
        run(n64);

        if(n64.size_change)
        {
            create_texture(n64.rdp.screen_x,n64.rdp.screen_y);
            n64.size_change = false;
        }
    }
    
    render(n64.rdp.screen.data());
}

void N64Window::debug_halt()
{
    n64.debug.debug_input();
}

void N64Window::core_throttle()
{

}

void N64Window::core_unbound()
{

}

void N64Window::handle_debug()
{
    if(n64.debug.is_halted())
    {
        n64.debug.debug_input();
    }
}