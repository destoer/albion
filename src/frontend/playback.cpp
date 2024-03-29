
#include "playback.h"
#ifdef AUDIO_SDL


#define SDL_MAIN_HANDLED
#ifdef _WIN32
#include <SDL.H>
#else
#include <SDL2/SDL.h>
#endif

void Playback::init(int playback_frequency,int sample_size) noexcept
{
    SDL_AudioSpec audio_spec;

	memset(&audio_spec,0,sizeof(audio_spec));

	audio_spec.freq = playback_frequency;
	audio_spec.format = AUDIO_F32SYS;
	audio_spec.channels = 2;
	audio_spec.samples = sample_size;	
	audio_spec.callback = NULL; // we will use SDL_QueueAudio()  rather than 
	audio_spec.userdata = NULL; // using a callback :)


    sample_idx = 0;

    audio_buf.resize(sample_size);

    SDL_OpenAudio(&audio_spec,NULL);
	start();
}

void Playback::mix_samples(float &f1, const float &f2, int volume) noexcept
{
    SDL_MixAudioFormat((Uint8*)&f1,(Uint8*)&f2,AUDIO_F32SYS,sizeof(float),volume);
}


void Playback::push_sample(const float &l, const float &r) noexcept
{
    if(sample_idx >= audio_buf.size())
    {
        push_samples();
        sample_idx = 0;
    }

    audio_buf[sample_idx] = l;
    audio_buf[sample_idx+1] = r;

    sample_idx += 2;
}

#include <thread>

void Playback::push_samples()
{
    // legacy interface
    static constexpr SDL_AudioDeviceID dev = 1;
    
    auto buffer_size = (audio_buf.size() * sizeof(float));

    // delay execution and let the que drain
    while(SDL_GetQueuedAudioSize(dev) > buffer_size)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }			


    if(SDL_QueueAudio(dev,audio_buf.data(),buffer_size) < 0)
    {
        printf("%s\n",SDL_GetError()); exit(1);
    }
}

void Playback::start() noexcept
{
	play_audio = true;
    SDL_PauseAudio(0);
}

void Playback::stop() noexcept
{
	play_audio = false;
    SDL_PauseAudio(1);
	SDL_ClearQueuedAudio(1);
}

Playback::~Playback()
{
    SDL_CloseAudio();
}


#else

//external handler
#ifdef AUDIO_ENABLE
static_assert(false,"no audio frontend defined!");
#endif

#endif


// stub audio playback helpers
#ifndef AUDIO_ENABLE
void Playback::init(int playback_frequency,int sample_size) noexcept
{
    UNUSED(playback_frequency); UNUSED(sample_size);
}

void Playback::mix_samples(float &f1, const float &f2,int volume) noexcept
{
    UNUSED(f1); UNUSED(f2); UNUSED(volume);
}

void Playback::push_sample(const float &l, const float &r) noexcept
{
    UNUSED(l); UNUSED(r);
}

void Playback::start() noexcept
{

}
void Playback::stop() noexcept
{

}

Playback::~Playback() 
{

}

void Playback::push_samples()
{

}
#endif
