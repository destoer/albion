#pragma once
#include <albion/lib.h>
#include <albion/input.h>


#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL3/SDL.h>
#endif

enum class emu_control
{
    throttle_t,
    unbound_t,
    break_t,
    quit_t,
    none_t,
};

class Input
{
public:
    void init();
    emu_control handle_input(SDL_Window* window, b32 ignore_key_inputs = false);
    void handle_controller_input();
    void add_event_from_key(s32 key, b32 down);

    ~Input();

    Controller controller;

private:
    bool connect_controller(SDL_JoystickID id);
    void disconnect_controller(SDL_JoystickID id);

    bool controller_connected = false;
    SDL_Gamepad *game_controller = NULL;
    SDL_JoystickID id = -1;
};
