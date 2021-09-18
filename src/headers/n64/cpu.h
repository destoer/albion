#pragma once
#include <n64/forward_def.h>
#include <destoer-emu/lib.h>
#include <n64/mips.h>

namespace nintendo64
{


struct Cpu
{
    u64 regs[32];
    u64 pc;
    u64 pc_next;

    u64 lo;
    u64 hi;

    u64 cp0_regs[32];


    // cp0 status
    bool ie;
    bool exl;
    bool erl;
    u32 ksu;
    bool ux;
    bool sx;
    bool kx;
    u8 im;
    u32 ds;
    bool re;
    bool fr;
    bool rp;

    // other control bits are unused
    bool cu1;
};


using INSTR_FUNC = void (*)(N64 &n64, u32 opcode);

void step(N64 &n64);
void reset_cpu(Cpu &cpu);

void skip_instr(Cpu &cpu);
void write_pc(N64 &n64, u64 pc);

void cycle_tick(N64 &n64, u32 cycles);

void write_cp0(Cpu &cpu, u64 v, u32 reg);

// instruction handlers
void instr_unknown(N64 &n64, u32 opcode);
void instr_unknown_cop0(N64 &n64, u32 opcode);
void instr_unknown_r(N64 &n64, u32 opcode);
void instr_unknown_regimm(N64 &n64, u32 opcode);

void instr_lui(N64 &n64, u32 opcode);
void instr_addiu(N64 &n64, u32 opcode);
void instr_addi(N64 &n64, u32 opcode);
void instr_ori(N64 &n64, u32 opcode);
void instr_andi(N64 &n64, u32 opcode);
void instr_xori(N64 &n64, u32 opcode);
void instr_jal(N64 &n64, u32 opcode);
void instr_slti(N64 &n64, u32 opcode);


void instr_lw(N64 &n64, u32 opcode);
void instr_sw(N64 &n64, u32 opcode);
void instr_lbu(N64 &n64, u32 opcode);
void instr_sb(N64 &n64, u32 opcode);


void instr_bne(N64 &n64, u32 opcode);
void instr_beq(N64 &n64, u32 opcode);
void instr_beql(N64 &n64, u32 opcode);
void instr_bnel(N64 &n64, u32 opcode);
void instr_blezl(N64 &n64, u32 opcode);
void instr_bgezl(N64 &n64, u32 opcode);


void instr_sll(N64 &n64, u32 opcode);
void instr_srl(N64 &n64, u32 opcode);
void instr_or(N64 &n64, u32 opcode);
void instr_jr(N64 &n64, u32 opcode);
void instr_sltu(N64 &n64, u32 opcode);
void instr_multu(N64 &n64, u32 opcode);
void instr_subu(N64 &n64, u32 opcode);
void instr_mflo(N64 &n64, u32 opcode);
void instr_cache(N64 &n64, u32 opcode);
void instr_addu(N64 &n64, u32 opcode);
void instr_and(N64 &n64, u32 opcode);
void instr_slt(N64 &n64, u32 opcode);
void instr_add(N64 &n64, u32 opcode);

void instr_cop0(N64 &n64, u32 opcode);
void instr_mtc0(N64 &n64, u32 opcode);
void instr_r_fmt(N64 &n64, u32 opcode);
void instr_regimm(N64 &n64, u32 opcode);


}