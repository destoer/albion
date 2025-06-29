#include "gba_window.h"

void GBAWindow::init(const std::string& filename, Playback& playback)
{
    init_sdl(gameboyadvance::SCREEN_WIDTH,gameboyadvance::SCREEN_HEIGHT);
    input.init();
    gba.reset(filename);	
    gba.apu.audio_buffer.playback = &playback;
    playback.init(gba.apu.audio_buffer);
}

void GBAWindow::pass_input_to_core()
{
    gba.handle_input(input.controller);
    input.controller.input_events.clear();
}

void GBAWindow::core_quit()
{
    gba.mem.save_cart_ram();
    exit(0);   
}

void GBAWindow::run_frame(bool paused)
{
    if(!paused)
    {
        gba.run();
    }
    render(gba.disp.screen.data());
}

void GBAWindow::debug_halt()
{
    gba.debug.debug_input();
}

void GBAWindow::core_throttle()
{
    playback.start();
    reset_audio_buffer(gba.apu.audio_buffer);
    gba.throttle_emu = true;
}

void GBAWindow::core_unbound()
{
    playback.stop();
    gba.throttle_emu = false; 
}

void GBAWindow::handle_debug()
{
    if(gba.debug.is_halted())
    {
        gba.debug.debug_input();
    }
}