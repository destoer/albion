#include "input.h"
#include <albion/lib.h>

void Input::init()
{
    int gamepad_count = 0;
    SDL_JoystickID *gamepad = SDL_GetJoysticks(&gamepad_count);

    // no gamepads
    if(!gamepad)
    {
        return;
    }

    // pick first valid controller
	for(int i = 0; i < gamepad_count; i++)
	{
        if(connect_controller(id))
        {
            break;
        }
	}  
}

bool Input::connect_controller(SDL_JoystickID id)
{
    if(SDL_IsGamepad(id) && !controller_connected)
    {
        game_controller = SDL_OpenGamepad(id);
        if(game_controller)
        {
            controller_connected = true;
            this->id = id;
            return true;
        }
    }

    return false;    
}

void Input::disconnect_controller(SDL_JoystickID id)
{
    if(this->id == id)
    {
        game_controller = NULL;
        controller_connected = false;
        this->id = -1;
    }
}

Input::~Input()
{
    if(controller_connected && game_controller != NULL)
    {
        // this is apparently buggy...
        //SDL_GameControllerClose(controller);
        game_controller = NULL;
    }
}


void Input::add_event_from_key(s32 key, b32 down)
{
	switch(key)
	{
		case SDLK_RETURN: controller.add_event(controller_input::start,down); break;
		case SDLK_SPACE: controller.add_event(controller_input::select,down); break;


		case SDLK_UP: controller.add_event(controller_input::up,down); break;
		case SDLK_DOWN: controller.add_event(controller_input::down,down); break;
		case SDLK_LEFT: controller.add_event(controller_input::left,down); break;
		case SDLK_RIGHT: controller.add_event(controller_input::right,down); break;

		case SDLK_A: controller.add_event(controller_input::a,down); break;
		case SDLK_S: controller.add_event(controller_input::x,down); break;
		case SDLK_D: controller.add_event(controller_input::left_trigger,down); break;
		case SDLK_F: controller.add_event(controller_input::right_trigger,down); break;

		default: break;
	}
}

/*
enum class emu_control
{
    debug_t,
    throttle,
    unbound_t,
    break_t,
    quit_t,
    none,
}

*/

emu_control Input::handle_input(SDL_Window* window, b32 ignore_key_inputs)
{
    b32 key_pressed = false;

	SDL_Event event;
	
	emu_control control = emu_control::none_t;

	// handle input
	while(SDL_PollEvent(&event))
	{
		switch(event.type) 
		{
			case SDL_EVENT_WINDOW_RESIZED:
			{
				SDL_SetWindowSize(window,event.window.data1, event.window.data2);
				break;
			}

			case SDL_EVENT_QUIT:
			{
				control = emu_control::quit_t;
				break;
			}	
			
			case SDL_EVENT_KEY_DOWN:
			{
				if(ignore_key_inputs)
				{
					break;
				}

				key_pressed = true;


				const s32 key = event.key.key;

				switch(key)
				{
					case SDLK_P:
					{
						control = emu_control::break_t;
						break;
					}

					case SDLK_K:
					{
						control = emu_control::unbound_t;
						break;
					}

					case SDLK_L:
					{
						control = emu_control::throttle_t;
						break;
					}

					default:
					{
						add_event_from_key(event.key.key,true);
						break;
					}
				}
                break;
			}
			
			case SDL_EVENT_KEY_UP:
			{
				if(!ignore_key_inputs)
				{
					add_event_from_key(event.key.key,false);
				}
                break;
			}

			case SDL_EVENT_GAMEPAD_ADDED: connect_controller(event.cdevice.which); break;
			case SDL_EVENT_GAMEPAD_REMOVED: disconnect_controller(event.cdevice.which); break;


            default:
            {
                break;
            }  
        }
    }

	// no keys pressed handle controller events
	if(!key_pressed)
	{
		handle_controller_input();
	}

	return control;
}


void get_joystick(SDL_Gamepad* game_controller,Joystick& stick,SDL_GamepadAxis x_axis, SDL_GamepadAxis y_axis)
{
    static constexpr u32 DEADZONE_LIM = 3200;

    stick.x = SDL_GetGamepadAxis(game_controller,x_axis);
    stick.y = SDL_GetGamepadAxis(game_controller,y_axis);   

    //printf("(%d,%d)\n",stick.x,stick.y);

    // not enough push on the controller dont register an input
    stick.in_deadzone = abs(stick.x) <= DEADZONE_LIM && abs(stick.y) <= DEADZONE_LIM;

    if(stick.in_deadzone)
    {
        stick.x = 0;
        stick.y = 0;
    }
}

void Input::handle_controller_input()
{
    // no controller we dont care
    if(!controller_connected)
    {
        return;
    }

    // get game controller update
    SDL_UpdateGamepads();

    static constexpr SDL_GamepadButton sdl_buttons[] = 
    {
        SDL_GAMEPAD_BUTTON_WEST,
        SDL_GAMEPAD_BUTTON_SOUTH,
        SDL_GAMEPAD_BUTTON_START,
        SDL_GAMEPAD_BUTTON_BACK,
        SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
        SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
    };

    static constexpr controller_input controller_buttons[] = 
    {
        controller_input::a,
        controller_input::x,
        controller_input::start,
        controller_input::select,
        controller_input::right_trigger,
        controller_input::left_trigger,
    };

    static constexpr int CONTROLLER_BUTTONS_SIZE = sizeof(controller_buttons) / sizeof(controller_buttons[0]);
    static_assert(CONTROLLER_BUTTONS_SIZE == (sizeof(sdl_buttons) / sizeof(sdl_buttons[0])));


    

    // cache the old state so we know when a new key press has occured
    static bool buttons_prev[CONTROLLER_BUTTONS_SIZE] = {false};

    // check the controller button state
    for(int i = 0; i < CONTROLLER_BUTTONS_SIZE; i++)
    {
        // now we handle controller inputs
        auto b = SDL_GetGamepadButton(game_controller,sdl_buttons[i]);
        if(b && !buttons_prev[i])
        {
			controller.add_event(controller_buttons[i],true);
        }

        else if(!b && buttons_prev[i])
        {
			controller.add_event(controller_buttons[i],false);
        }
        buttons_prev[i] = b;
    }


    // input of more than half in either direction is enough to make
    // to cause an input
    constexpr int16_t threshold = std::numeric_limits<int16_t>::max() / 2;

    // NOTE: this is effectively a digital input for the anlog stick and should be ignored by the handler when we
    // actually want an input the stick directly

    // if something is greater than threshold and not pushed before
    // key press or if it was and now isnt release the key
    // do for all 4 keys
    if(controller.simulate_dpad)
    {
        static constexpr int LEFT = 0;
        static constexpr int RIGHT = 1;
        static constexpr int UP = 2;
        static constexpr int DOWN = 3;
        static bool prev_dpad[4] = 
        {
            false, // left
            false, // right
            false, // up
            false // down
        };

        // handle the joystick
        const auto x = SDL_GetGamepadAxis(game_controller,SDL_GAMEPAD_AXIS_LEFTX);
        const auto y = SDL_GetGamepadAxis(game_controller,SDL_GAMEPAD_AXIS_LEFTY);

        // in y axis deadzone deset both
        if(y == threshold)
        {
            if(prev_dpad[DOWN])
            {
                controller.add_event(controller_input::down,false);
            }

            if(prev_dpad[UP])
            {
                controller.add_event(controller_input::up,false);
            }
            prev_dpad[LEFT] = false;
            prev_dpad[RIGHT] = false;
        }

        // in x axis deadzone deset both
        if(x == threshold)
        {
            if(prev_dpad[LEFT])
            {
                controller.add_event(controller_input::left,false);
            }

            if(prev_dpad[RIGHT])
            {
                controller.add_event(controller_input::right,false);
            }
            prev_dpad[LEFT] = false;
            prev_dpad[RIGHT] = false;
        }


        const bool r = x > threshold;
        const bool l = x < -threshold;
        const bool u = y < -threshold;
        const bool d = y > threshold;


        // right
        if(prev_dpad[RIGHT] != r)
        {
            controller.add_event(controller_input::right,r);
            prev_dpad[RIGHT] = r;
        }

        // left
        if(prev_dpad[LEFT] != l)
        {
            controller.add_event(controller_input::left,l);
            prev_dpad[LEFT] = l;
        }

        // up
        if(prev_dpad[UP] != u)
        {
            controller.add_event(controller_input::up,u);
            prev_dpad[UP] = u;    
        }

        // down
        if(prev_dpad[DOWN] != d)
        {
            controller.add_event(controller_input::down,d);
            prev_dpad[DOWN] = d;    
        }

    }

    else
    {
        get_joystick(game_controller,controller.left,SDL_GAMEPAD_AXIS_LEFTX,SDL_GAMEPAD_AXIS_LEFTY);
    }

    // handle analog triggers
    const auto trig_l = SDL_GetGamepadAxis(game_controller,SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    const auto trig_r = SDL_GetGamepadAxis(game_controller,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

    static bool trig_l_prev = false;
    static bool trig_r_prev = false;

    const bool trig_l_cur = trig_l > threshold;
    const bool trig_r_cur = trig_r > threshold;

    if(trig_l_prev != trig_l_cur)
    {
		controller.add_event(controller_input::left_trigger,trig_l_cur);
        trig_l_prev = trig_l_cur;
    }

    if(trig_r_prev != trig_r_cur)
    {
		controller.add_event(controller_input::right_trigger,trig_r_cur);
        trig_r_prev = trig_r_cur;
    }

}
