#include <gba/gba.h>

namespace gameboyadvance
{
GBAScheduler::GBAScheduler(GBA &gba) : cpu(gba.cpu), disp(gba.disp), 
    apu(gba.apu), mem(gba.mem)
{
    init();
}

// option to align cycles for things like waitloop may be required
void GBAScheduler::skip_to_event()
{
    auto cycles = min_timestamp - timestamp;

    cpu.cycle_tick(cycles);
} 


// better way to handle this? std::function is slow
void GBAScheduler::service_event(const EventNode<gba_event> &node)
{
    // if its double speed we need to push half the cycles
    // through the function even though we delay for double
    const auto cycles_to_tick = timestamp - node.start;

    switch(node.type)
    {
        case gba_event::sample_push:
        {
            apu.push_samples(cycles_to_tick);
            break;
        }

        case gba_event::timer0:
        {
            cpu.tick_timer(0,cycles_to_tick);
            break;
        }

        case gba_event::timer1:
        {
            cpu.tick_timer(1,cycles_to_tick);
            break;
        }

        case gba_event::timer2:
        {
            cpu.tick_timer(2,cycles_to_tick);
            break;
        }

        case gba_event::timer3:
        {
            cpu.tick_timer(3,cycles_to_tick);
            break;
        }

        case gba_event::display:
        {
            disp.tick(cycles_to_tick);
            break;
        }
    }
}


}