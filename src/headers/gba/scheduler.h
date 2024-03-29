#pragma once
#include <gba/forward_def.h>
#include <albion/lib.h>
#include <albion/debug.h>
#include <albion/scheduler.h>

namespace gameboyadvance
{

// just easy to put here
enum class gba_event
{
    sample_push,
    c1_period_elapse,
    c2_period_elapse,
    c3_period_elapse,
    c4_period_elapse,
    psg_sequencer,
    timer0,
    timer1,
    timer2,
    timer3,
    display
};

constexpr size_t EVENT_SIZE = 11;

struct GBAScheduler final : public Scheduler<EVENT_SIZE,gba_event>
{
    GBAScheduler(GBA &gba);


    void skip_to_event();

    Cpu &cpu;
    Display &disp;
    Apu &apu;
    Mem &mem;

protected:
    void service_event(const EventNode<gba_event> & node) override;
};

}