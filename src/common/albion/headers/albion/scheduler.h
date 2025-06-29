#pragma once
#include<albion/min_heap.h>

// needs a save state impl
template<size_t EVENT_SIZE,typename event_type>
class Scheduler
{
public:
    void init();

    void save_state(std::ofstream &fp);
    dtr_res load_state(std::ifstream &fp);    

    void tick(uint32_t cycles);
    void delay_tick(uint32_t cycles);
    bool is_active(event_type t) const;
    bool event_ready() const;
    void service_events();

    std::optional<EventNode<event_type>> get(event_type t) const;
    std::optional<size_t> get_event_ticks(event_type t) const;

    void insert(const EventNode<event_type> &node, bool tick_old=true);

    // remove events of the specifed type
    void remove(event_type type,bool tick_old=true);

    uint64_t get_timestamp() const;

    uint64_t get_next_event_cycles() const;

    size_t size() const
    { 
        return event_list.size(); 
    } 

    // helper to create events
    EventNode<event_type> create_event(u64 duration, event_type t) const;

    void adjust_timestamp();

protected:
    virtual void service_event(const EventNode<event_type> & node) = 0;


    MinHeap<EVENT_SIZE,event_type> event_list;

    // current elapsed time
    u64 timestamp = 0;
    u64 min_timestamp = 0;
};



template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::init()
{
    event_list.clear();
    timestamp = 0;
    min_timestamp = 0xffffffff;
}

template<size_t SIZE,typename event_type>
bool Scheduler<SIZE,event_type>::is_active(event_type t) const
{
    return event_list.is_active(t);
}
template<size_t SIZE,typename event_type>
bool Scheduler<SIZE,event_type>::event_ready() const
{
    return timestamp >= min_timestamp;
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::delay_tick(uint32_t cycles)
{
    timestamp += cycles;
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::service_events()
{
    // if the timestamp is greater than the event fire
    // handle the event and remove it
    // as the list is sorted the next event is first
    // any subsequent events aernt going to fire so we can return early
    while(event_list.size() && event_ready())
    {
        // remove min event
        const auto event = event_list.peek();
        event_list.pop();
        min_timestamp = event_list.peek().end;
        service_event(event);
    }
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::tick(uint32_t cycles)
{
    timestamp += cycles;

    service_events();
}

// using a 64 bit timestamp not required
/*
template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::adjust_timestamp()
{
    // timestamp will soon overflow
    if(is_set(timestamp,31))
    {
        uint32_t min = 0xffffffff;
        //puts("timestamp overflow");
        for(const auto &x: event_list.buf)
        {
            if(event_list.is_active(x.type))
            {
                min = std::min(min,x.start);
            }
        }

        for(auto &x: event_list.buf)
        {
            if(event_list.is_active(x.type))
            {
                x.end -= min;
                x.start -= min;
            }
        }
        timestamp -= min;
        min_timestamp -= min;
    }    
}
*/

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::insert(const EventNode<event_type> &node,bool tick_old)
{
    remove(node.type,tick_old);
    event_list.insert(node);
    min_timestamp = event_list.peek().end;
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::remove(event_type type,bool tick_old)
{
    const auto event = event_list.remove(type);

    // if it was removed and we want to tick off cycles
    if(event && tick_old)
    {
        service_event(event.value());
        event_list.remove(type);
    }
    min_timestamp = event_list.peek().end;
}

template<size_t SIZE,typename event_type>
std::optional<EventNode<event_type>> Scheduler<SIZE,event_type>::get(event_type t) const
{
    return event_list.get(t);
}

template<size_t SIZE,typename event_type>
std::optional<size_t> Scheduler<SIZE,event_type>::get_event_ticks(event_type t) const
{
    const auto event = get(t);

    if(!event)
    {
        return std::nullopt;
    }

    return timestamp - event.value().start;
}

template<size_t SIZE,typename event_type>
uint64_t Scheduler<SIZE,event_type>::get_timestamp() const
{
    return timestamp;
}

template<size_t SIZE,typename event_type>
uint64_t Scheduler<SIZE,event_type>::get_next_event_cycles() const
{
    return min_timestamp - timestamp;
}

template<size_t SIZE,typename event_type>
EventNode<event_type> Scheduler<SIZE,event_type>::create_event(u64 duration, event_type t) const
{
    return EventNode<event_type>(timestamp,duration+timestamp,t);
}

template<size_t SIZE,typename event_type>
void Scheduler<SIZE,event_type>::save_state(std::ofstream &fp)
{
    file_write_var(fp,min_timestamp);
    file_write_var(fp,timestamp);
    event_list.save_state(fp);
}

template<size_t SIZE,typename event_type>
dtr_res Scheduler<SIZE,event_type>::load_state(std::ifstream &fp)
{
    dtr_res err = file_read_var(fp,min_timestamp);
    err |= file_read_var(fp,timestamp);
    err |= event_list.load_state(fp);

    return err;
}
