#pragma once
#include <albion/lib.h>
#include <gba/arm.h>
#include <gba/interrupt.h>

namespace gameboyadvance
{

struct HaltCnt
{
    HaltCnt();
    void init();

    // write only
    void write(u8 v);


    enum class power_state
    {
        halt,
        stop,
        normal
    };

    power_state state;
};


struct Timer
{
    Timer(int timer);

    void init();

    u8 read_counter(int idx) const;

    // actually writes the reload but is at the same addr
    void write_counter(int idx, u8 v);

    u8 read_control() const;
    void write_control(u8 v);

    // counter
    u16 reload = 0;
    u16 counter = 0;


    int cycle_count = 0;

    // control
    int scale = 0;
    bool count_up = false;
    bool irq = false;
    bool enable = false;


    static constexpr int cycle_limit[4] = {1,64,256,1024};
    static constexpr int shift_table[4] = {0,6,8,10};
    static constexpr interrupt timer_interrupts[4] = 
    {
        interrupt::timer0,
        interrupt::timer1,
        interrupt::timer2,
        interrupt::timer3
    };

    const interrupt timer_interrupt;

};

// cpu io registers
struct CpuIo
{
    CpuIo();
    void init();


    // interrupt master enable
    bool ime = false;
    u16 interrupt_enable = 0;
    u16 interrupt_flag = 0;
    HaltCnt halt_cnt;


    std::array<Timer,4> timers{0,1,2,3};
};

} 