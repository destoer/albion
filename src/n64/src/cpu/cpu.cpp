#include <n64/n64.h>

#include "cpu/cop0.cpp"
#include "cpu/cop1.cpp"

namespace nintendo64
{

std::string disass_n64(N64& n64, Opcode opcode, u64 addr)
{
    return beyond_all_repair::disass_mips(n64.program,addr,opcode);
}

template<const b32 debug>
void call_handler(N64& n64, const Opcode& opcode, u32 idx)
{
    if constexpr(debug)
    {
        INSTR_TABLE_DEBUG[idx](n64,opcode);
    }

    else
    {
        INSTR_TABLE_NO_DEBUG[idx](n64,opcode);
    }    
}

void reset_cpu(N64 &n64)
{
    auto &cpu = n64.cpu;

    // setup regs to hle the pif rom
    memset(cpu.regs,0,sizeof(cpu.regs));
    cpu.regs[beyond_all_repair::T3] = 0xFFFFFFFFA4000040;
    cpu.regs[beyond_all_repair::S4] = 0x0000000000000001;
    cpu.regs[beyond_all_repair::S6] = 0x000000000000003F;
    cpu.regs[beyond_all_repair::SP] = 0xFFFFFFFFA4001FF0;

    auto& cop0 = cpu.cop0;
    cop0 = {};
    

    cop0.random = 0x0000001F;
    write_cop0(n64,0,beyond_all_repair::COUNT);
    write_cop0(n64,0,beyond_all_repair::COMPARE);
    write_cop0(n64,0xffff'ffff,beyond_all_repair::EPC);
    write_cop0(n64,0xffff'ffff,beyond_all_repair::ERROR_EPC);
    write_cop0(n64,0x3400'0000,beyond_all_repair::STATUS);

    cpu.cop1 = {};

    cpu.pc = 0xA4000040;
    cpu.pc_fetch = cpu.pc;
    cpu.pc_next = cpu.pc + 4; 

    insert_count_event(n64);
}


void cycle_tick(N64 &n64, u32 cycles)
{
    n64.scheduler.delay_tick(cycles);
    update_random(n64.cpu.cop0);
}




template<const b32 debug>
void step(N64 &n64)
{   
    const branch_delay_state branch_delay_next[] = 
    {
        branch_delay_state::during,
        branch_delay_state::end,
        branch_delay_state::end,
    };

    n64.cpu.branch_delay = branch_delay_next[u32(n64.cpu.branch_delay)];

    n64.cpu.pc_fetch = n64.cpu.pc;

    if((n64.cpu.pc & 3) != 0)
    {
        address_error_exception(n64,n64.cpu.pc_fetch,address_error::load);
        return;
    }

    const auto pc_phys_addr_opt = translate_vaddr(n64,n64.cpu.pc_fetch,tlb_access::read);

    // invalid vaddr exception raised
    if(!pc_phys_addr_opt) 
    {
        return;
    }

    const auto pc_phys_addr = *pc_phys_addr_opt;
    
    const u32 op = read_u32_physical<debug>(n64,pc_phys_addr);

    if constexpr(debug)
    {
        if(n64.debug.breakpoint_hit(u32(pc_phys_addr),op,break_type::execute))
        {
            // halt until told otherwhise :)
            n64.debug.print_console("execute breakpoint hit ({:x}:{:x})\n",n64.cpu.pc,op);
            n64.debug.halt();
            return;
        }
    }

    skip_instr(n64.cpu);

    const Opcode opcode = beyond_all_repair::make_opcode(op);

    // std::cout << fmt::format("{:16x}: {}\n",n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc_next));
    
    // call the instr handler
    //const u32 offset = beyond_all_repair::get_opcode_type(opcode.op);
    const u32 offset = beyond_all_repair::calc_base_table_offset(opcode);
    
    call_handler<debug>(n64,opcode,offset);
    
    // $zero is hardwired to zero, make sure writes cant touch it
    n64.cpu.regs[beyond_all_repair::R0] = 0;

    // assume 1 CPI
    // TODO: i dont anything should have such sensitive timings yet..
    cycle_tick(n64,1);
}

void write_pc(N64 &n64, u64 pc)
{
    if((pc & 0b11) != 0)
    {
        spdlog::warn("PC address invalid {:x}",pc);
    }

    n64.debug.trace.add(n64.cpu.pc,pc);	

    n64.cpu.pc_next = pc;
}

void write_pc_delayed(N64& n64, u64 pc)
{
    write_pc(n64,pc);
    n64.cpu.branch_delay = branch_delay_state::start;
}

void write_call(N64 &n64, u64 pc)
{
    write_pc_delayed(n64,pc);
    n64.debug.last_call = pc;
}


void skip_instr(Cpu &cpu)
{
    cpu.pc = cpu.pc_next;
    cpu.pc_next += beyond_all_repair::MIPS_INSTR_SIZE;
}

}