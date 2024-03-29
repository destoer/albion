#pragma once
#include <albion/lib.h>

namespace gameboyadvance
{
struct ApuIo;

struct SoundCnt
{
    SoundCnt(ApuIo &apu_io);
    void init();

    void write_l(int idx,u8 v);
    u8 read_l(int idx) const;

    void write_h(int idx, u8 v);
    u8 read_h(int idx) const;

    void write_x(int idx, u8 v);
    u8 read_x(int idx);

    // sound cnt L
    int vol_right;
    int vol_left;
    int right_enable;
    int left_enable;

    // soundcnt h
    int psg_vol;
    bool dma_vol_a;
    bool dma_vol_b;

    bool enable_right_a;
    bool enable_left_a;
    bool timer_num_a;


    bool enable_right_b;
    bool enable_left_b;
    bool timer_num_b;


    // soundcnt x
    bool sound1_enable;
    bool sound2_enable;
    bool sound3_enable;
    bool sound4_enable;
    bool sound_enable;


    ApuIo &apu_io;
};



struct SoundFifo
{
    SoundFifo();
    void init();


    void write(int8_t v);
    int8_t read(); // not used by the memory handler only interrnally

    int len;
    int read_idx;
    int write_idx;
    int8_t fifo[32];
};

struct ApuIo
{
    ApuIo();
    void init();

    SoundCnt sound_cnt{*this};
    SoundFifo fifo_a;
    SoundFifo fifo_b;
    u16 soundbias = 0;
};

}