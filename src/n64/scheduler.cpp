#include <n64/n64.h>

namespace nintendo64
{

void N64Scheduler::skip_to_event()
{
    timestamp = min_timestamp;
    service_events();
}

void N64Scheduler::service_event(const EventNode<n64_event> & node)
{
    const auto cycles_to_tick = timestamp - node.start;

    switch(node.type)
    {
        case n64_event::line_inc:
        {
            increment_line(n64);
            break;
        }

        case n64_event::count:
        {
            count_event(n64,cycles_to_tick);
            break;
        }

        case n64_event::ai_dma:
        {
            audio_event(n64);
            break;
        }

        case n64_event::si_dma:
        {
            si_dma_finished(n64);
            break;
        }

        case n64_event::pi_dma:
        {
            pi_dma_finished(n64);
            break;
        }

        case n64_event::sp_dma:
        {
            sp_dma_finished(n64);
            break;
        }
    }
}

}