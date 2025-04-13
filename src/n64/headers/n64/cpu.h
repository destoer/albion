#pragma once
#include <n64/forward_def.h>
#include <albion/lib.h>
#include <n64/cpu/mips.h>
#include <n64/cpu/cop0.h>
#include <n64/cpu/cop1.h>
#include <beyond_all_repair.h>

namespace nintendo64
{

enum class branch_delay_state
{
    start,
    during,
    end
};


struct Cpu
{
    u64 regs[32];
    u64 pc;
    u64 pc_next;
    u64 pc_fetch;

    u64 lo;
    u64 hi;

    Cop0 cop0;
    Cop1 cop1;

    b32 interrupt = false;
    branch_delay_state branch_delay = branch_delay_state::end;
};


using Opcode = beyond_all_repair::Opcode;

using INSTR_FUNC = void (*)(N64 &n64, const Opcode &opcode);

void reset_cpu(Cpu &cpu);

void skip_instr(Cpu &cpu);
void write_pc(N64 &n64, u64 pc);
void write_call(N64& n64, u64 pc);

void cycle_tick(N64 &n64, u32 cycles);

void write_cop0(N64 &n64, u64 v, u64 reg);
u64 read_cop0(N64& n64, u32 reg);


void instr_unknown_opcode(N64 &n64, const Opcode &opcode);

const u32 KERNEL_MODE = 0b00;
const u32 SUPERVISOR_MODE = 0b01;
const u32 USER_MODE = 0b10;

void dump_instr(N64& n64);

}