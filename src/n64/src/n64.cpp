#include <n64/n64.h>
#include <n64/cpu/mips_lut.h>

// TODO:
// make dmas not complete instantly, but rather memcpy at the end

// unity build
#include "mem/mem.cpp"
#include "cpu/cpu.cpp"
#include "mem/layout.cpp"
#include "instr/instr.cpp"
#include "instr/mips_lut.cpp"
#include "rcp/rdp.cpp"
#include "debug.cpp"
#include "scheduler.cpp"

namespace nintendo64
{

b32 read_func(beyond_all_repair::Program& program,u64 addr,void* out, u32 size)
{
    auto& n64 = *(N64*)program.data;

    switch(size)
    {
        case 1:
        {
            const u8 v = read_mem_raw<u8>(n64,addr);
            memcpy(out,&v,size);
            break;
        }

        case 2:
        {
            const u16 v = read_mem_raw<u16>(n64,addr);
            memcpy(out,&v,size);
            break;            
        }

        case 4:
        {
            const u32 v = read_mem_raw<u32>(n64,addr);
            memcpy(out,&v,size);
            break;                    
        }

        default: return false;
    }
    

    return true;
}

const char* reg_name(u32 idx) 
{
    return beyond_all_repair::REG_NAMES[idx];
}


void reset(N64 &n64, const std::string &filename)
{
    reset_mem(n64.mem,filename);
    reset_cpu(n64);
    reset_rdp(n64);
    n64.size_change = false;

    // initializer external disassembler
    n64.program = beyond_all_repair::make_program(0xA4000040,false,&read_func,&n64);

    n64.audio_buffer = make_audio_buffer();
    reset_audio_buffer(n64.audio_buffer);

    spdlog::info("N64 Emulation Core initialized.");
}

template<const b32 debug>
void run_internal(N64 &n64)
{
    n64.rdp.frame_done = false;

    // dont know how our vblank setup works
    while(!n64.rdp.frame_done)
    {

        while(!n64.scheduler.event_ready())
        {
            if constexpr(debug)
            {
                if(n64.debug.is_halted())
                {
                    return;
                }
            }
            step<debug>(n64);
        }
        n64.scheduler.service_events();
    }

    // dont know when the rendering should be finished just do at end for now
    render(n64);
}


void run(N64& n64)
{
    if(n64.debug_enabled)
    {
        run_internal<true>(n64);
    }

    else
    {
        run_internal<false>(n64);
    }
}

void dump_instr(N64& n64)
{
    const auto opcode = read_mem_raw<u32>(n64,n64.cpu.pc_fetch);
    const auto op = beyond_all_repair::make_opcode(opcode);
    spdlog::debug("{:16x}: {}\n",n64.cpu.pc_fetch,disass_n64(n64,op,n64.cpu.pc_next));
}

}