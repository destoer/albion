#pragma once
#include <gb/gb.h>
#include "sdl_window.h"

class GameboyWindow final : public SDLMainWindow
{
protected:
    void init(const std::string& filename,Playback& playback) override;
    void pass_input_to_core() override;
    void run_frame(bool paused) override;
    void handle_debug() override;
    void core_quit() override;
    void core_throttle() override;
    void core_unbound() override;
    void debug_halt() override;

private:
    gameboy::GB gb;
};
