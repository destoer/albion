#include <n64/n64.h>

namespace nintendo64
{
    
b32 cop2_usable(N64& n64)
{
    // for now hack around this
    auto& status = n64.cpu.cop0.status;

    if(!status.cu2)
    {
        coprocesor_unusable(n64,2);
        return false;
    }

    return true;
}

void instr_unknown_cop2(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] unknown cop2 opcode {:08x} {:08x}\n",n64.cpu.pc,disass_n64(n64,opcode,n64.cpu.pc),opcode.op,(opcode.op >> 21) & 0b11111);
    n64.debug.trace.print();
    throw std::runtime_error(err);    
}

template<const b32 debug>
void instr_COP2(N64 &n64, const Opcode &opcode)
{
    const u32 offset = beyond_all_repair::calc_cop2_table_offset(opcode);

    call_handler<debug>(n64,opcode,offset);
}

template<const b32 debug>
void instr_lwc2(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

template<const b32 debug>
void instr_ldc2(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

template<const b32 debug>
void instr_swc2(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

template<const b32 debug>
void instr_sdc2(N64 &n64, const Opcode &opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

void instr_mfc2(N64& n64, const Opcode& opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

void instr_dmfc2(N64& n64, const Opcode& opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

void instr_cfc2(N64& n64, const Opcode& opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

void instr_mtc2(N64& n64, const Opcode& opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

void instr_dmtc2(N64& n64, const Opcode& opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}

void instr_ctc2(N64& n64, const Opcode& opcode)
{
    // coprocesor unusable
    if(!cop2_usable(n64))
    {
        return;
    }

    instr_unknown_cop2(n64,opcode);
}


}