#include <n64/n64.h>

namespace nintendo64 {
const INSTR_FUNC instr_lut[] = {
&instr_r_fmt,
&instr_regimm,
&instr_j,
&instr_jal,
&instr_beq,
&instr_bne,
&instr_unknown,
&instr_bgtz,
&instr_addi,
&instr_addiu,
&instr_slti,
&instr_unknown,
&instr_andi,
&instr_ori,
&instr_xori,
&instr_lui,
&instr_cop0,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_beql,
&instr_bnel,
&instr_blezl,
&instr_unknown,
&instr_daddi,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_lb,
&instr_unknown,
&instr_unknown,
&instr_lw,
&instr_lbu,
&instr_unknown,
&instr_unknown,
&instr_lwu,
&instr_sb,
&instr_sh,
&instr_unknown,
&instr_sw,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_cache,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_ld,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_unknown,
&instr_sd,
};


const DISASS_FUNC disass_lut[] = {
&disass_r_fmt,
&disass_regimm,
&disass_j,
&disass_jal,
&disass_beq,
&disass_bne,
&disass_unknown,
&disass_bgtz,
&disass_addi,
&disass_addiu,
&disass_slti,
&disass_unknown,
&disass_andi,
&disass_ori,
&disass_xori,
&disass_lui,
&disass_cop0,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_beql,
&disass_bnel,
&disass_blezl,
&disass_unknown,
&disass_daddi,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_lb,
&disass_unknown,
&disass_unknown,
&disass_lw,
&disass_lbu,
&disass_unknown,
&disass_unknown,
&disass_lwu,
&disass_sb,
&disass_sh,
&disass_unknown,
&disass_sw,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_cache,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_ld,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_unknown,
&disass_sd,
};


const INSTR_FUNC instr_cop0_lut[] = {
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_mtc0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
&instr_unknown_cop0,
};


const DISASS_FUNC disass_cop0_lut[] = {
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_mtc0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
&disass_unknown_cop0,
};


const INSTR_FUNC instr_r_lut[] = {
&instr_sll,
&instr_unknown_r,
&instr_srl,
&instr_unknown_r,
&instr_sllv,
&instr_unknown_r,
&instr_srlv,
&instr_unknown_r,
&instr_jr,
&instr_jalr,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_sync,
&instr_unknown_r,
&instr_unknown_r,
&instr_mflo,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_multu,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_add,
&instr_addu,
&instr_unknown_r,
&instr_subu,
&instr_and,
&instr_or,
&instr_xor,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_slt,
&instr_sltu,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
&instr_unknown_r,
};


const DISASS_FUNC disass_r_lut[] = {
&disass_sll,
&disass_unknown_r,
&disass_srl,
&disass_unknown_r,
&disass_sllv,
&disass_unknown_r,
&disass_srlv,
&disass_unknown_r,
&disass_jr,
&disass_jalr,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_sync,
&disass_unknown_r,
&disass_unknown_r,
&disass_mflo,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_multu,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_add,
&disass_addu,
&disass_unknown_r,
&disass_subu,
&disass_and,
&disass_or,
&disass_xor,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_slt,
&disass_sltu,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
&disass_unknown_r,
};


const INSTR_FUNC instr_regimm_lut[] = {
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_bgezl,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_bgezal,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
&instr_unknown_regimm,
};


const DISASS_FUNC disass_regimm_lut[] = {
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_bgezl,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_bgezal,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
&disass_unknown_regimm,
};


}
