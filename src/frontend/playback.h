#pragma once
#include <destoer/destoer.h>
#include <albion/audio.h>

#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL3/SDL.h>
#endif

class Playback
{
public:
    void init(AudioBuffer& buffer) noexcept;

    bool is_playing() const noexcept { return play_audio; }

    void start() noexcept;
    void stop() noexcept;

    ~Playback();
    void push_samples(AudioBuffer& audio_buffer);

private:
    SDL_AudioStream *stream;
    bool play_audio = false;
};