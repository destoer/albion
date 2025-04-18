#include <n64/n64.h>

namespace nintendo64
{

void instr_unknown_regimm(N64 &n64, const Opcode &opcode)
{
    const auto err = fmt::format("[cpu {:16x} {}] regimm unknown opcode {:08x}\n",n64.cpu.pc-4,disass_n64(n64,opcode,n64.cpu.pc),opcode.op);
    n64.debug.trace.print();
    throw std::runtime_error(err);        
}

template<const b32 debug>
void instr_REGIMM(N64& n64, const Opcode& opcode)
{
    if constexpr(debug)
    {
        INSTR_TABLE_DEBUG[beyond_all_repair::REGIMM_OFFSET + ((opcode.op >> beyond_all_repair::REGIMM_SHIFT) & beyond_all_repair::REGIMM_MASK)](n64,opcode);
    }

    else
    {
        INSTR_TABLE_NO_DEBUG[beyond_all_repair::REGIMM_OFFSET + ((opcode.op >> beyond_all_repair::REGIMM_SHIFT) & beyond_all_repair::REGIMM_MASK)](n64,opcode);
    }    
}

void instr_bgez(N64& n64, const Opcode& opcode)
{
    instr_branch(n64,opcode,branch_kind::normal,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) >= 0;
    }); 
}

void instr_bgezl(N64 &n64, const Opcode &opcode)
{
    instr_branch_likely(n64,opcode,branch_kind::normal,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) >= 0;
    }); 
}

void instr_bgezal(N64 &n64, const Opcode &opcode)
{
    instr_branch(n64,opcode,branch_kind::linked,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) >= 0;
    });
}

void instr_bgezall(N64 &n64, const Opcode &opcode)
{

    instr_branch_likely(n64,opcode,branch_kind::linked,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) >= 0;
    });
}

void instr_bltz(N64 &n64, const Opcode &opcode)
{
    instr_branch(n64,opcode,branch_kind::normal,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) < 0;
    });   
}

void instr_bltzl(N64 &n64, const Opcode &opcode)
{
    instr_branch_likely(n64,opcode,branch_kind::normal,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) < 0;
    }); 
}

void instr_bltzal(N64 &n64, const Opcode &opcode)
{
    instr_branch(n64,opcode,branch_kind::linked,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) < 0;
    });   
}

void instr_bltzall(N64 &n64, const Opcode &opcode)
{
    instr_branch_likely(n64,opcode,branch_kind::linked,[](N64& n64, const Opcode& opcode)
    {
        return s64(n64.cpu.regs[opcode.rs]) < 0;
    }); 
}

void raise_trap(N64& n64)
{
    standard_exception(n64,beyond_all_repair::TRAP);
}

void trap_cmp(N64& n64, b32 cmp_res)
{
    if(cmp_res)
    {
        raise_trap(n64);
    }
}

void instr_tge(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,s64(n64.cpu.regs[opcode.rs]) >= s64(n64.cpu.regs[opcode.rt]));
}

void instr_tgeu(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] >= n64.cpu.regs[opcode.rt]);
}

void instr_tlt(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,s64(n64.cpu.regs[opcode.rs]) < s64(n64.cpu.regs[opcode.rt]));
}

void instr_tltu(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] < n64.cpu.regs[opcode.rt]);
}

void instr_teq(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] == n64.cpu.regs[opcode.rt]);
}

void instr_tne(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] != n64.cpu.regs[opcode.rt]);
}


void instr_tlti(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,s64(n64.cpu.regs[opcode.rs]) < s16(opcode.imm));
}

void instr_tltiu(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] < u64(s16(opcode.imm)));
}


void instr_tgtiu(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] > opcode.imm);
}


void instr_tgei(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,s64(n64.cpu.regs[opcode.rs]) >= s16(opcode.imm));
}

void instr_tgeiu(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,n64.cpu.regs[opcode.rs] >= u64(s16(opcode.imm)));
}

void instr_teqi(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,s64(n64.cpu.regs[opcode.rs]) == s16(opcode.imm));
}

void instr_tnei(N64 &n64, const Opcode &opcode)
{
    trap_cmp(n64,s64(n64.cpu.regs[opcode.rs]) != s16(opcode.imm));
}


}