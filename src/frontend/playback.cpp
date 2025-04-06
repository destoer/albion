
#include "playback.h"
#include <algorithm>


void Playback::init(AudioBuffer& buffer) noexcept
{
    UNUSED(buffer);
    SDL_AudioSpec audio_spec;

	memset(&audio_spec,0,sizeof(audio_spec));

	audio_spec.freq = AUDIO_BUFFER_SAMPLE_RATE;
	audio_spec.format = SDL_AUDIO_F32;
	audio_spec.channels = AUDIO_CHANNEL_COUNT;
    
    this->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&audio_spec,nullptr,nullptr);

    if(!this->stream) 
    {
        spdlog::error("Failed to open audio {}",SDL_GetError());
    }
	stop();
}

void Playback::start() noexcept
{
	play_audio = true;
    SDL_ResumeAudioStreamDevice(stream);
}

void Playback::stop() noexcept
{
	play_audio = false;
    SDL_PauseAudioStreamDevice(stream);
	SDL_ClearAudioStream(stream);
}

Playback::~Playback()
{
    if(stream)
    {
        // This causes crashes during an assert (i have no idea why).
        // SDL_DestroyAudioStream(stream);
        stream = nullptr;
    }
}

void push_samples(Playback* playback,AudioBuffer& audio_buffer)
{
    playback->push_samples(audio_buffer);
}

void Playback::push_samples(AudioBuffer& audio_buffer)
{
    if(!play_audio)
    {
        return;
    }

    //printf("pushing: %ld : %ld : %ld\n",audio_buffer.length,audio_buffer.buffer.size(),SDL_GetQueuedAudioSize(dev) / sizeof(f32));

    const u32 buffer_size = audio_buffer.length * sizeof(f32);

    // delay execution and let the queue drain
    while(SDL_GetAudioStreamQueued(stream) > (int)buffer_size)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }			

    if(!SDL_PutAudioStreamData(stream,audio_buffer.buffer.data(),buffer_size))
    {
        printf("Failed to queue audio %s\n",SDL_GetError()); exit(1);
    }
}
