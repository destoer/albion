#pragma once
#include "forward_def.h"
#include <destoer-emu/lib.h>
#include <destoer-emu/debug.h>

namespace gameboy
{

class Cpu
{
public:
    void init(Memory *m, Ppu *p,Apu *ap, Disass *d, Debug *debugger);
    void step();
    void cycle_tick(int cycles) noexcept; 


    void request_interrupt(int interrupt) noexcept;
    bool get_cgb() const noexcept {return is_cgb;}


    uint16_t internal_timer = 0;
    
    uint8_t joypad_state = 0xff;



    /* register reads for the debugger */

    uint16_t get_pc() const noexcept { return pc; }
    uint16_t get_sp() const noexcept { return sp; }
    uint16_t get_hl() const noexcept { return read_hl(); }
    uint16_t get_af() const noexcept { return read_af(); }
    uint16_t get_bc() const noexcept { return read_bc(); }
    uint16_t get_de() const noexcept { return read_de(); }

    // save states
    void save_state(std::ofstream &fp);
    void load_state(std::ifstream &fp);

private:
    Memory *mem = nullptr;
    Ppu *ppu = nullptr;
    Apu *apu = nullptr;
    Disass *disass = nullptr;
    Debug *debug = nullptr;


    // registers
    uint8_t a; uint8_t f; //af
    uint8_t b; uint8_t c; //bc
    uint8_t d; uint8_t e; //de
    uint8_t h; uint8_t l; //hl
    uint16_t sp;
    uint16_t pc;


    // interrupts
    bool halt = false;
    bool ei = false;
    bool di = false;
    bool interrupt_enable = false;
    bool halt_bug = false;


    // timer
    void update_timers(int cycles) noexcept;
    



    // cgb
    bool is_cgb = false;
    bool is_double = false;

    void write_af(uint16_t data) noexcept;
    void write_bc(uint16_t data) noexcept;
    void write_de(uint16_t data) noexcept;
    void write_hl(uint16_t data) noexcept;

    uint16_t read_af() const noexcept;
    uint16_t read_de() const noexcept;
    uint16_t read_bc() const noexcept;
    uint16_t read_hl() const noexcept;



    void exec_instr();
    void exec_cb(uint8_t cbop);
    void handle_instr_effects();


    // interrupts
    void service_interrupt(int interrupt) noexcept;
    void do_interrupts() noexcept;

    // instr helpers
    void set_zero(uint8_t reg) noexcept;
    void instr_dec(uint8_t reg) noexcept;
    void instr_inc(uint8_t reg) noexcept;
    void instr_bit(uint8_t reg, uint8_t bit) noexcept;
    void instr_and(uint8_t num) noexcept;
    uint8_t instr_rl(uint8_t reg) noexcept;
    void instr_sub(uint8_t num) noexcept;
    void instr_cp(uint8_t num) noexcept;
    void instr_sbc(uint8_t num) noexcept;
    void instr_add(uint8_t num) noexcept;
    uint16_t instr_addi(uint16_t reg, int8_t num) noexcept;
    void instr_adc(uint8_t num) noexcept;
    uint16_t instr_addw(uint16_t reg, uint16_t num) noexcept;
    void instr_or(uint8_t val) noexcept;
    uint8_t instr_swap(uint8_t num) noexcept;
    void instr_xor(uint8_t num) noexcept;
    uint8_t instr_sla(uint8_t reg) noexcept;
    uint8_t instr_sra(uint8_t reg) noexcept;
    uint8_t instr_srl(uint8_t reg) noexcept;
    uint8_t instr_rr(uint8_t reg) noexcept;
    uint8_t instr_rrc(uint8_t reg) noexcept;
    uint8_t instr_rlc(uint8_t reg) noexcept;
    void instr_jr() noexcept;
    void instr_jr_cond(bool cond, int bit) noexcept;
    void call_cond(bool cond, int bit) noexcept;
    void ret_cond(bool cond, int bit) noexcept;


    // stack helpers
    uint8_t read_stackt() noexcept;
    void write_stackt(uint8_t v) noexcept;
    uint16_t read_stackwt() noexcept;
    void write_stackwt(uint16_t v) noexcept;
    void write_stackw(uint16_t v) noexcept;
    void write_stack(uint8_t v) noexcept;

    void check_rst_loop(uint16_t addr, uint8_t op);

};


constexpr uint32_t Z = 7; // zero flag
constexpr uint32_t N = 6; // negative flag
constexpr uint32_t H = 5; // half carry flag
constexpr uint32_t C = 4; // carry flag

}