#include <psg/psg.h>

namespace gameboy_psg
{

Psg::Psg()
{

}

void Psg::init(psg_mode mode, bool use_bios)
{
    this->mode = mode;
    



    // init every channel
    init_channels(mode,channels);
    init_sweep(sweep);
    init_noise(noise);
    init_wave(wave,mode);


    enable_sound();
    if(!use_bios)
    {
        nr10 = 0x80;
        nr11 = 0xBF;
        nr12 = 0xF3;
        nr13 = 0x00;
        nr14 = 0xBF;


        nr21 = 0x3f;
        nr22 = 0;
        nr33 = 0;
        nr24 = 0xbf;

        nr30 = 0x7f;
        nr31 = 0xff;
        nr32 = 0x9f;
        nr33 = 0x00;
        nr34 = 0xbf;

        nr41 = 0xff;
        nr22 = 0;
        nr43 = 0;
        nr44 = 0xbf;

        nr50 = 0x77;
        nr51 = 0xF3;
        nr52 = 0xF1;
    }

    else
    {
        nr10 = 0x00;
        nr11 = 0x00;
        nr12 = 0x00;
        nr13 = 0x00;
        nr14 = 0x00;


        nr21 = 0x00;
        nr22 = 0x00;
        nr33 = 0x00;
        nr24 = 0x00;

        nr30 = 0x00;
        nr31 = 0x00;
        nr32 = 0x00;
        nr33 = 0x00;
        nr34 = 0x00;

        nr41 = 0x00;
        nr22 = 0x00;
        nr43 = 0x00;
        nr44 = 0x00;

        nr50 = 0x00;
        nr51 = 0x00;
        nr52 = 0x00;
    }
}

void Psg::advance_sequencer() noexcept
{
	// switch and perform the function required for our step
	switch(sequencer_step)
	{
		case 0: // clock the length counters
		{
			tick_length_counters();
			break;
		}
			
		case 1: break; // do nothing

			
		case 2: // sweep generator + lengthc
		{
			tick_length_counters();
			clock_sweep(*this);
			break;
		}
			
		case 3: break; // do nothing

			
		case 4: // clock lengthc
		{
			tick_length_counters();
			break;
		}
			
		case 5: break; // do nothing
			
		case 6:  // clock the sweep generator + lengthc
		{
			tick_length_counters();
			clock_sweep(*this);
			break;
		}
			
		case 7:
		{ 
			clock_envelopes();
			break; //clock the envelope 
		}
	}

	// go to the next step
	sequencer_step = (sequencer_step + 1) & 7;
}

void Psg::reset_sequencer() noexcept
{
    sequencer_step = 0;
}

void Psg::tick_length_counters() noexcept
{
    tick_length_counters_internal(channels);
}

void Psg::clock_envelopes() noexcept
{
    clock_envelope(channels[0]);
    clock_envelope(channels[1]);
    clock_envelope(channels[3]);
}

void Psg::tick_periods(u32 cycles) noexcept
{
    square_tick_period(channels[0],cycles);
    square_tick_period(channels[1],cycles);
    wave_tick_period(wave,channels[2],cycles);
    noise_tick_period(noise,channels[3],cycles);    
}

void Psg::enable_sound() noexcept
{
    nr52 |= 0x80; // data had 0x80 so write back  
    sound_enabled = true;

    // reset length coutners when powerd up
    // if on cgb
    if(mode != psg_mode::dmg)
    {
        for(int i = 0; i < 4; i++)
        {
            channels[i].lengthc = 0;
        }
    }

    sequencer_step = 0;

    // reset every duty (NOTE: noise has a dummy one)
    for(int i = 0; i < 4; i++)
    {
        channels[i].duty_idx = 0;
    }
}

void Psg::disable_sound() noexcept
{
    // set nr10-nr51 regs to 0
    write_nr10(0);
    write_nr11(0);
    write_nr12(0);
    write_nr13(0);
    write_nr14(0);

    write_nr21(0);
    write_nr22(0);
    write_nr23(0);
    write_nr24(0);
 
    write_nr30(0);
    write_nr31(0);
    write_nr32(0);
    write_nr33(0);
    write_nr34(0);

    write_nr41(0);
    write_nr42(0);
    write_nr43(0);
    write_nr44(0);

    write_nr50(0);
    write_nr51(0);


    nr52 = 112; // need to write the unused bits and just zero everything else

    // now lock writes
    sound_enabled = false;     
}

void Psg::write_nr10(u8 v) noexcept
{	
    if(sound_enabled)
    {
        sweep_write(sweep,channels[0],v);
        nr10 = v | 128;
    }
}

u8 Psg::read_nr10() const noexcept
{
    return nr10;
}

void Psg::write_nr11(u8 v) noexcept
{
    if(sound_enabled || mode == psg_mode::dmg)
    {
        // bottom 6 bits are length data 
        // set the internal counter to 64 - bottom 6 bits of data
        write_lengthc(channels[0],v);
        // can only be written while on both versions
        if(sound_enabled)
        {
            write_cur_duty(channels[0],v);
            nr11 = v;
        }
    }    
}

u8 Psg::read_nr11() const noexcept
{
    return (nr11 & (128 + 64)) | (0xff-(128+64));
}

void Psg::write_nr12(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr12 = v;
        channels[0].dac_on = dac_masks[0] & nr12;

        check_dac(channels[0]);
        env_write(channels[0],v);
    }
}

u8 Psg::read_nr12() const noexcept
{
    return nr12;
}

void Psg::write_nr13(u8 v) noexcept
{
    if(sound_enabled)
    {
        freq_write_lower(channels[0],v);
        nr13 = v;
    }    
}

void Psg::write_nr14(u8 v) noexcept
{
    if(sound_enabled)
    {
        freq_write_higher(channels[0],v);
        

        if(is_set(v,7)) // trigger
        {
            length_trigger(channels[0]);
            freq_trigger(channels[0]);
            env_trigger(channels[0]);
            sweep_trigger(sweep,channels[0]);
            duty_trigger(channels[0]);
        }


        length_write(channels[0],v,sequencer_step);
        nr14 = v;

        // after all the trigger events have happend
        // if the dac is off switch channel off				
        check_dac(channels[0]);
    }    
}

u8 Psg::read_nr14() const noexcept
{
    return (nr14 & (64)) | (0xff-64);  
}

void Psg::write_nr21(u8 v) noexcept
{
    if(sound_enabled || mode == psg_mode::dmg)
    {
        write_lengthc(channels[1],v);
        if(sound_enabled)
        {
            write_cur_duty(channels[1],v);
            nr21 = v;
        }
    }    
}

u8 Psg::read_nr21() const noexcept
{
    return (nr21 & (128 + 64)) | (0xff-(128+64));	
}

void Psg::write_nr22(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr22 = v;
        channels[1].dac_on = dac_masks[1] & nr22;

        check_dac(channels[1]);	
        env_write(channels[1],v);
    }    
}

u8 Psg::read_nr22() const noexcept
{
    return nr22;
}

void Psg::write_nr23(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr23 = v;
        freq_write_lower(channels[1],v);
    }    
}

void Psg::write_nr24(u8 v) noexcept
{
    if(sound_enabled)
    {
        freq_write_higher(channels[1],v);


        if(is_set(v,7)) // trigger
        {
            length_trigger(channels[1]);
            freq_trigger(channels[1]);
            env_trigger(channels[1]);
            duty_trigger(channels[1]);
        }

        length_write(channels[1],v,sequencer_step);
        nr24 = v;


        check_dac(channels[1]);	
    }    
}

u8 Psg::read_nr24() const noexcept
{
    return (nr24 & (64)) | (0xff-64);	  
}

void Psg::write_nr30(u8 v) noexcept
{
    if(sound_enabled)
    {
    
        if(mode == psg_mode::gba)
        {
            wave.dimension = is_set(v,5);
            wave.bank_idx = is_set(v,6);            
        }

        nr30 = v | 127;

        channels[2].dac_on = dac_masks[2] & nr30;
        check_dac(channels[2]);
    }    
}

u8 Psg::read_nr30() const noexcept
{
    const u8 base = (nr30 & (128)) | (0xff-128);

    if(mode == psg_mode::gba)
    {
        return base | wave.dimension << 5 | wave.bank_idx << 6;
    }

    return base;
}

void Psg::write_nr31(u8 v) noexcept
{
    if(sound_enabled || mode == psg_mode::dmg)
    {
        write_lengthc(channels[2],v);
        nr31 = v;
    }    
}

void Psg::write_nr32(u8 v) noexcept
{
    if(sound_enabled)
    {
        wave_write_vol(channels[2],v);
        nr32 = v | 159;
    }    
}

u8 Psg::read_nr32() const noexcept
{
    return (nr32 & (64 + 32)) | (0xff-(64+32));
}

void Psg::write_nr33(u8 v) noexcept
{
    if(sound_enabled)
    {
        freq_write_lower(channels[2],v);
        nr33 = v;
    }    
}

void Psg::write_nr34(u8 v) noexcept
{
    if(sound_enabled)
    {
        freq_write_higher(channels[2],v);

        if(is_set(v,7)) // trigger
        {
            length_trigger(channels[2]);
            freq_trigger(channels[2]);
            wave_trigger(channels[2]);
            wave_vol_trigger(channels[2]);
        }

        length_write(channels[2],v,sequencer_step);
        nr34 = v | (16 + 32 + 8);

        check_dac(channels[2]);	
    }
}

u8 Psg::read_nr34() const noexcept
{
    return (nr34 & (64)) | (0xff-64);
}

void Psg::write_nr41(u8 v) noexcept
{
    if(sound_enabled || mode == psg_mode::dmg)
    {
        write_lengthc(channels[3],v);
        nr41 = v | 192;
    }
}

void Psg::write_nr42(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr42 = v;
        channels[3].dac_on = dac_masks[3] & nr42;
        check_dac(channels[3]);
        env_write(channels[3],v);
    }
}

u8 Psg::read_nr42() const noexcept
{
    return nr42;
}

void Psg::write_nr43(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr43 = v;
        noise_write(noise,v);
    }
}

u8 Psg::read_nr43() const noexcept
{
    return nr43;
}


void Psg::write_nr44(u8 v) noexcept
{
    if(sound_enabled)
    {
        if(is_set(v,7)) // trigger
        {
            length_trigger(channels[3]);
            env_trigger(channels[3]);
            noise_trigger(noise);
        }

        length_write(channels[3],v,sequencer_step);		
        nr44 = v | 63;

        check_dac(channels[3]);	
    }
}

u8 Psg::read_nr44() const noexcept
{
    return (nr44 & (64)) | (0xff-64);	
}




void Psg::write_nr50(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr50 = v;
    }
}

u8 Psg::read_nr50() const noexcept
{
    return nr50;
}
    
void Psg::write_nr51(u8 v) noexcept
{
    if(sound_enabled)
    {
        nr51 = v;
    }
}

u8 Psg::read_nr51() const noexcept
{
    return nr51;
}



u8 Psg::read_nr52() const noexcept
{
    return (nr52 & 0xf0) | 
        channels[0].enabled << 0 |
        channels[1].enabled << 1 |
        channels[2].enabled << 2 |
        channels[3].enabled << 3;
}


void Psg::nr1_freq_writeback(u16 v) noexcept
{
    // write back low 8
    nr13 = v & 0xff;

    
    // and high 3
    nr14 &= ~0x7; // mask bottom 3
    nr14 |= (v >> 8) & 0x7; // and write them out    
}

void Psg::write_wave_table(int idx, u8 v) noexcept
{
    assert(idx < 0x20);

    // if wave is on write to current byte <-- finish accuracy later (dmg)
    if(chan_enabled(2))
    {
        if(mode == psg_mode::gba)
        {
            wave.table[!wave.bank_idx][(channels[2].duty_idx / 2)] = v;
        }

        else if(mode == psg_mode::cgb)
        {
            wave.table[0][(channels[2].duty_idx / 2)] = v;
        }
    }

    else // if its off allow "free reign" over it
    {
        if(mode == psg_mode::gba)
        {
            wave.table[!wave.bank_idx][idx] = v;	
        }

        else
        {
            wave.table[0][idx] = v;
        }
    }
}

u8 Psg::read_wave_table(int idx) const noexcept
{
    assert(idx < 0x20);

    // if wave is on write to current byte <-- finish accuracy later
    if(chan_enabled(2))
    {
        // can only access on dmg when the wave channel is...
        // todo
        if(mode == psg_mode::cgb)
        {
            return wave.table[0][channels[2].duty_idx / 2];
        }

        else if(mode == psg_mode::gba)
        {
            return wave.table[!wave.bank_idx][channels[2].duty_idx / 2];
        }

        else
        {
            return 0xff;
        }
    }
    
    // if its off allow "free reign" over it
    if(mode != psg_mode::gba)
    {
        return wave.table[0][idx];
    }

    else
    {
        return wave.table[!wave.bank_idx][idx];
    }    
}

void channel_save_state(Channel &c, std::ofstream &fp)
{
	file_write_var(fp,c);
}

void sweep_save_state(Sweep &s, std::ofstream &fp)
{
	file_write_var(fp,s);
}

void wave_save_state(Wave &w, std::ofstream &fp)
{
	file_write_var(fp,w);
}

void noise_save_state(Noise &n, std::ofstream &fp)
{
	file_write_var(fp,n);
}

dtr_res channel_load_state(Channel &c, std::ifstream &fp)
{
	const auto res = file_read_var(fp,c);
    c.duty_idx &= 7;
    c.cur_duty &= 3;

    return res;
}

dtr_res sweep_load_state(Sweep &s, std::ifstream &fp)
{
	return file_read_var(fp,s);
}

dtr_res wave_load_state(Wave &w, std::ifstream &fp)
{
	return file_read_var(fp,w);
}

dtr_res noise_load_state(Noise &n, std::ifstream &fp)
{
	const auto res = file_read_var(fp,n);
    n.divisor_idx &= 7;
    return res;
}




void Psg::save_state(std::ofstream &fp)
{
	file_write_var(fp,mode);


	file_write_var(fp,sound_enabled);

	file_write_var(fp,sequencer_step);

	// backing regs

	// nr1x
	file_write_var(fp,nr10);
	file_write_var(fp,nr11);
    file_write_var(fp,nr12);
	file_write_var(fp,nr13);
	file_write_var(fp,nr14);

	// nr2x
    file_write_var(fp,nr21);
	file_write_var(fp,nr22);
	file_write_var(fp,nr23);
	file_write_var(fp,nr24);

	// nr3x
	file_write_var(fp,nr30);
	file_write_var(fp,nr31);
	file_write_var(fp,nr32);
	file_write_var(fp,nr33);
	file_write_var(fp,nr34);

	// nr4x
	file_write_var(fp,nr41);
	file_write_var(fp,nr42);
	file_write_var(fp,nr43);
	file_write_var(fp,nr44);	

	// nr5x
	file_write_var(fp,nr50);
	file_write_var(fp,nr51);
	file_write_var(fp,nr52);

    // save channel data
    for(int i = 0; i < 4; i++)
    {
        channel_save_state(channels[i],fp);
    }

    wave_save_state(wave,fp);
    noise_save_state(noise,fp);
    sweep_save_state(sweep,fp);
}

dtr_res Psg::load_state(std::ifstream &fp)
{
	dtr_res err = file_read_var(fp,mode);


	err |= file_read_var(fp,sound_enabled);

	err |= file_read_var(fp,sequencer_step);

	// backing regs

	// nr1x
	err |= file_read_var(fp,nr10);
	err |= file_read_var(fp,nr11);
    err |= file_read_var(fp,nr12);
	err |= file_read_var(fp,nr13);
	err |= file_read_var(fp,nr14);

	// nr2x
    err |= file_read_var(fp,nr21);
	err |= file_read_var(fp,nr22);
	err |= file_read_var(fp,nr23);
	err |= file_read_var(fp,nr24);

	// nr3x
	err |= file_read_var(fp,nr30);
	err |= file_read_var(fp,nr31);
	err |= file_read_var(fp,nr32);
	err |= file_read_var(fp,nr33);
	err |= file_read_var(fp,nr34);

	// nr4x
	err |= file_read_var(fp,nr41);
	err |= file_read_var(fp,nr42);
	err |= file_read_var(fp,nr43);
	err |= file_read_var(fp,nr44);	

	// nr5x
	err |= file_read_var(fp,nr50);
	err |= file_read_var(fp,nr51);
	err |= file_read_var(fp,nr52);

    // load in channel data
    for(int i = 0; i < 4; i++)
    {
        err |= channel_load_state(channels[i],fp);
    }

    err |= wave_load_state(wave,fp);
    err |= noise_load_state(noise,fp);
    err |= sweep_load_state(sweep,fp);

    return err;
}

}