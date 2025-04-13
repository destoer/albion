#include "n64/mem.h"
#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_cop0(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown cop0 opcode {:08x}\n",n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),(opcode.op >> 21) & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

template<const b32 debug>
void instr_COP0(N64 &n64, const Opcode &opcode)
{
    const u32 offset = calc_cop0_table_offset(opcode);

    call_handler<debug>(n64,opcode,offset);
}

void instr_mtc0(N64 &n64, const Opcode &opcode)
{
    write_cop0(n64,sign_extend_mips<s64,s32>(n64.cpu.regs[opcode.rt]),opcode.rd); 
}

void instr_dmtc0(N64 &n64, const Opcode &opcode)
{
    write_cop0(n64,n64.cpu.regs[opcode.rt],opcode.rd); 
}

void instr_mfc0(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rt] = sign_extend_type<s64,s32>(read_cop0(n64,opcode.rd)); 
}

void instr_dmfc0(N64 &n64, const Opcode &opcode)
{
    n64.cpu.regs[opcode.rt] = read_cop0(n64,opcode.rd); 
}

template<const b32 debug>
void instr_sc(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    const auto vaddr = n64.cpu.regs[base] + imm;

    if(n64.cpu.cop0.ll_bit)
    {
        const auto phys_addr_opt = translate_aligned_addr_write<u32>(n64,vaddr);

        if(!phys_addr_opt)
        {
            return;
        }
        
        const auto phys_addr = *phys_addr_opt;

        write_u32_physical<debug>(n64,phys_addr,n64.cpu.regs[opcode.rt]);   
    }

    n64.cpu.regs[opcode.rt] = n64.cpu.cop0.ll_bit;
}


template<const b32 debug>
void instr_scd(N64 &n64, const Opcode &opcode)
{
    const auto base = opcode.rs;
    const auto imm = sign_extend_mips<s64,s16>(opcode.imm);
    const auto vaddr = n64.cpu.regs[base] + imm;

    if(n64.cpu.cop0.ll_bit)
    {
        const auto phys_addr_opt = translate_aligned_addr_write<u64>(n64,vaddr);

        if(!phys_addr_opt)
        {
            return;
        }
        
        const auto phys_addr = *phys_addr_opt;

        write_u64_physical<debug>(n64,phys_addr,n64.cpu.regs[opcode.rt]);
    }

    n64.cpu.regs[opcode.rt] = n64.cpu.cop0.ll_bit;
}

void instr_eret(N64& n64, const Opcode& opcode)
{
    UNUSED(opcode);

    auto& cop0 = n64.cpu.cop0;
    auto& status = cop0.status;

    if(status.erl)
    {
        write_pc(n64,cop0.error_epc);
        status.erl = false;
    }

    else
    {
        write_pc(n64,cop0.epc);
        status.exl = false;
    }

    n64.cpu.cop0.ll_bit = false;

    // this does not execute the delay slot
    skip_instr(n64.cpu);

    check_interrupts(n64);
}

}