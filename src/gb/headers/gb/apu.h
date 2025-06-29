#pragma once
#include <albion/lib.h>
#include <albion/audio.h>
#include <gb/forward_def.h>
#include <gb/mem_constants.h>
#include <gb/scheduler.h>
#include <psg/psg.h>


namespace gameboy
{


struct Apu
{	
	Apu(GB &gb);

	void insert_new_sample_event() noexcept;

	void push_samples(u32 cycles) noexcept;

	void init(gameboy_psg::psg_mode mode, bool use_bios) noexcept;

	void tick(u32 cycles) noexcept;

	void disable_sound() noexcept;
	void enable_sound() noexcept;

	void save_state(std::ofstream &fp);
	dtr_res load_state(std::ifstream &fp);

	void insert_chan1_period_event()
	{
		insert_period_event(psg.channels[0].period,gameboy_event::c1_period_elapse);
	}

	void insert_chan2_period_event()
	{
		insert_period_event(psg.channels[1].period,gameboy_event::c2_period_elapse);
	}

	void insert_chan3_period_event()
	{
		insert_period_event(psg.channels[2].period,gameboy_event::c3_period_elapse);
	}

	void insert_chan4_period_event()
	{
		insert_period_event(psg.channels[3].period,gameboy_event::c4_period_elapse);
	}

	gameboy_psg::Psg psg;
	AudioBuffer audio_buffer;

	bool is_cgb;

	void insert_period_event(int period, gameboy_event chan) noexcept
	{
		// create  a new event as the period has changed
		// need to half the ammount if we are in double speed
		const auto event = scheduler.create_event(period << scheduler.is_double(),chan);

		// dont tick off the old event as 
		// it will use the new value as we have just overwritten 
		// the old internal counter
		// this is not an event we are dropping and expecting to start later 

		scheduler.insert(event,false);
	}

	GameboyScheduler &scheduler;

	// counter used to down sample	
	s32 down_sample_cnt = 0; 

	static constexpr int DOWN_SAMPLE_LIMIT = (4 * 1024 * 1024) / AUDIO_BUFFER_SAMPLE_RATE;
};

}