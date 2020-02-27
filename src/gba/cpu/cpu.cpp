#include <gba/cpu.h>
#include <gba/memory.h>
#include <gba/display.h>
#include <gba/disass.h>
#include <limits.h>

namespace gameboyadvance
{

void Cpu::init(Display *disp, Mem *mem, Debug *debug, Disass *disass)
{
    // init components
    this->disp = disp;
    this->mem = mem;
    this->debug = debug;
    this->disass = disass;

    // backup stores
    memset(user_regs,0,sizeof(user_regs));

    // r8 - r12 banked
    memset(fiq_banked,0,sizeof(fiq_banked)); 

    // regs 13 and 14 banked
    memset(hi_banked,0,sizeof(hi_banked));

    // banked status regs
    memset(status_banked,0,sizeof(status_banked));


    // setup main cpu state
    is_thumb = false;  // cpu in arm mode
    regs[PC] = 0x08000000; // cartrige reset vector
    regs[LR] = 0x08000000;
    cpsr = 0x1f;
    regs[SP] = 0x03007f00;
    hi_banked[static_cast<int>(cpu_mode::supervisor)][0] = 0x03007FE0;
    hi_banked[static_cast<int>(cpu_mode::irq)][0] = 0x03007FA0;
    //arm_fill_pipeline(); // fill the intitial cpu pipeline
    //regs[PC] = 0;
    arm_mode = cpu_mode::system;
    switch_mode(cpu_mode::system);
    init_opcode_table();

    dma_in_progress = false;
    cyc_cnt = 0;
    cpu_io.init();
}


void Cpu::init_opcode_table()
{
    init_arm_opcode_table();
    init_thumb_opcode_table();
}

void Cpu::init_thumb_opcode_table()
{
    thumb_opcode_table.resize(256);

    for(int i = 0; i < 256; i++)
    {

        // THUMB.1: move shifted register
        // top 3 bits unset
        if(((i >> 5) & 0b111) == 0b000 && ((i >> 3) & 0b11) != 0b11)
        {
            thumb_opcode_table[i] = &Cpu::thumb_mov_reg_shift;
        }

        // THUMB.2: add/subtract
        else if(((i >> 3) & 0b11111) == 0b00011)
        {
            thumb_opcode_table[i] = &Cpu::thumb_add_sub;
        }



        // THUMB.3: move/compare/add/subtract immediate
        else if(((i >> 5) & 0b111) == 0b001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_mcas_imm;
        }


        // THUMB.4: ALU operations
        else if(((i >> 2) & 0b111111) == 0b010000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_alu;
        }

        // THUMB.5: Hi register operations/branch exchange
        else if(((i >> 2) & 0b111111) == 0b010001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_hi_reg_ops;
        }

        // THUMB.6: load PC-relative
        else if(((i >> 3) & 0b11111) ==  0b01001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_ldr_pc;
        }


        // THUMB.7: load/store with register offset
        else if(((i >> 4) & 0b1111) == 0b0101 && !is_set(i,1))
        {
           thumb_opcode_table[i] = &Cpu::thumb_load_store_reg;
        }

        // THUMB.8: load/store sign-extended byte/halfword
        else if(((i >> 4) & 0b1111) == 0b0101 && is_set(i,1))
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_sbh;
        }

        // THUMB.9: load/store with immediate offset
        else if(((i>>5) & 0b111) == 0b011)
        {
            thumb_opcode_table[i] = &Cpu::thumb_ldst_imm;
        }



        //THUMB.10: load/store halfword
        else if(((i >> 4) & 0b1111) == 0b1000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_half;
        }

        // THUMB.11: load/store SP-relative
        else if(((i >> 4) & 0b1111) == 0b1001)
        {
            thumb_opcode_table[i] = &Cpu::thumb_load_store_sp;
        }

        // THUMB.12: get relative address
        else if(((i >> 4) & 0b1111) == 0b1010)
        {
            thumb_opcode_table[i] = &Cpu::thumb_get_rel_addr;
        }
        


        // THUMB.13: add offset to stack pointer
        else if(i == 0b10110000)
        {
            thumb_opcode_table[i] = &Cpu::thumb_sp_add;
        }



        //THUMB.14: push/pop registers
        else if(((i >> 4) & 0b1111) == 0b1011 
            && ((i >> 1) & 0b11) == 0b10)
        {
            thumb_opcode_table[i] = &Cpu::thumb_push_pop;
        }

        //  THUMB.15: multiple load/store
        else if(((i >> 4) & 0b1111) == 0b1100)
        {
            thumb_opcode_table[i] = &Cpu::thumb_multiple_load_store;
        }

        // THUMB.16: conditional branch
        else if(((i >> 4)  & 0b1111) == 0b1101 && (i & 0xf) != 0xf)
        {
            thumb_opcode_table[i] = &Cpu::thumb_cond_branch;
        }

        // THUMB.17: software interrupt and breakpoint
        else if(i == 0b11011111)
        {
            thumb_opcode_table[i] = &Cpu::thumb_swi;
        }


        // THUMB.18: unconditional branch
        else if(((i >> 3) & 0b11111) == 0b11100)
        {
            thumb_opcode_table[i] = &Cpu::thumb_branch;
        }
 
        // THUMB.19: long branch with link
        else if(((i >> 4) & 0b1111) == 0b1111)
        {
            thumb_opcode_table[i] = &Cpu::thumb_long_bl;
        }

        else 
        {
            thumb_opcode_table[i] = &Cpu::thumb_unknown;
        }                 
    }
}

void Cpu::init_arm_opcode_table()
{
    arm_opcode_table.resize(4096);


    for(int i = 0; i < 4096; i++)
    {
        switch(i >> 10) // bits 27 and 26 of opcode
        {
            case 0b00:
            {

                // 001
                if(is_set(i,9)) 
                {
                    int op = (i >> 5) & 0xf;

                    // msr and mrs
                    // ARM.6: PSR Transfer
                    // bit 24-23 must be 10 for this instr 
                    // bit 20 must also be zero
                    
                    // check it ocupies the unused space for
                    //TST,TEQ,CMP,CMN with a S of zero                    
                    if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                    {
                        arm_opcode_table[i] = &Cpu::arm_psr;
                    }

                    //  ARM.5: Data Processing 00 at bit 27
                    // arm data processing immediate
                    else
                    {
                        arm_opcode_table[i] = &Cpu::arm_data_processing;
                    }
                }

                // 000
                else 
                {
                    //ARM.3: Branch and Exchange
                    // bx
                    if(i == 0b000100100001) 
                    {
                        arm_opcode_table[i] = &Cpu::arm_branch_and_exchange;
                    }

                    // this section of the decoding needs improving....
                    else if((i & 0b1001) == 0b1001)
                    {
                        // ARM.7: Multiply and Multiply-Accumulate (MUL,MLA)
                        if(((i >> 6) & 0b111) == 0b000 && (i & 0xf) == 0b1001)
                        {
                            arm_opcode_table[i] = &Cpu::arm_mul;
                        }

                        // ARM.7: Multiply and Multiply-Accumulate (MUL,MLA) (long)
                        else if(((i >> 7) & 0b11) == 0b01 && (i & 0xf) == 0b1001)
                        {
                            arm_opcode_table[i] = &Cpu::arm_mull;                            
                        }
                        

                        // Single Data Swap (SWP)  
                        else if(is_set(i,8) && (i & 0xf) == 0b1001) // bit 24 set
                        {
                            arm_opcode_table[i] = &Cpu::arm_swap;
                        }

                        // ARM.10: Halfword, Doubleword, and Signed Data Transfer
                        //else if()
                        else 
                        {
                            arm_opcode_table[i] = &Cpu::arm_hds_data_transfer;
                        }
                    }

                    // psr or data processing
                    else
                    {
                        int op = (i >> 5) & 0xf;
                        // check it ocupies the unused space for
                        //TST,TEQ,CMP,CMN with a S of zero 
                        // ARM.6: PSR Transfer                   
                        if(op >= 0x8 && op <= 0xb && !is_set(i,4))
                        {
                            arm_opcode_table[i] = &Cpu::arm_psr;
                        }

                        //  ARM.5: Data Processing 00 at bit 27
                        // arm data processing register
                        else
                        {
                            arm_opcode_table[i] = &Cpu::arm_data_processing;
                        } 
                    }                   
                }
                break;
            }

            case 0b01:
            {
                //ARM.9: Single Data Transfer
                if(true) // assume for now
                {
                    arm_opcode_table[i] = &Cpu::arm_single_data_transfer;   
                }

                else 
                {
                    arm_opcode_table[i] = &Cpu::arm_unknown;
                }
                break;
            }

            case 0b10:
            {

                // 101 (ARM.4: Branch and Branch with Link)
                if(is_set(i,9))
                {
                    arm_opcode_table[i] = &Cpu::arm_branch;
                }


                // 100
                // ARM.11: Block Data Transfer (LDM,STM)
                else if(!is_set(i,9))
                {
                    arm_opcode_table[i] = &Cpu::arm_block_data_transfer;
                }
                break;
            }

            case 0b11:
            {
                arm_opcode_table[i] = &Cpu::arm_unknown;
                break;
            }
        } 
    }

}

void Cpu::cycle_tick(int cycles)
{
    disp->tick(cycles);
    tick_timers(cycles);
}

// lets ignore timers for now
void Cpu::tick_timers(int cycles)
{

    UNUSED(cycles);
    //static constexpr uint32_t timer_lim[4] = {1,64,256,1024};
    //static constexpr interrupt interrupt_table[4] = {interrupt::timer0,interrupt::timer1,interrupt::timer2,interrupt::timer3};
 /*
    // ignore count up timing for now
    for(int i = 0; i < 4; i++)
    {
        int offset = i*ARM_WORD_SIZE;
        uint16_t cnt = mem->handle_read<uint16_t>(mem->io,IO_TM0CNT_H+offset);

        if(!is_set(cnt,7)) // timer is not enabled
        {
            continue;
        }

        if(is_set(cnt,2)) // count up timer
        {
            throw std::runtime_error("[unimplemented] count up timer!");
        }

        uint32_t lim = timer_lim[cnt & 0x3];

       // printf("%08x\n",lim);

        timer_scale[i] += cycles;

        if(timer_scale[i] >= lim)
        {
            timers[i] += timer_scale[i] / lim;
            uint32_t max_cyc = std::numeric_limits<uint16_t>::max();
            if(timers[i] >= max_cyc) // overflowed
            {
                // add the reload values
                timers[i] = mem->handle_read<uint16_t>(mem->io,IO_TM0CNT_L+offset);
                if(is_set(cnt,6))
                {
                    request_interrupt(interrupt_table[i]);
                }
            }
            timer_scale[i] %= lim;
        }
    }  
*/  
}


// get this booting into armwrestler
// by skipping the state forward
void Cpu::step()
{

#ifdef DEBUG
    const uint32_t pc = regs[PC];
    uint32_t v = is_thumb? mem->read_mem<uint16_t>(pc) : mem->read_mem<uint32_t>(pc);
	if(debug->step_instr || debug->breakpoint_hit(pc,v,break_type::execute))
	{
		// halt until told otherwhise :)
		write_log("[DEBUG] execute breakpoint hit ({:x}:{:x})",pc,v);
		debug->halt();
	}
#endif


    if(is_thumb) // step the cpu in thumb mode
    {
        exec_thumb();
    }

    else // step the cpu in arm mode
    {
        exec_arm();
    }

    // handle interrupts
    do_interrupts();
}

// start here
// debug register printing
void Cpu::print_regs()
{

    // update current registers
    // so they can be printed
    store_registers(arm_mode);


    printf("current mode: %s\n",mode_names[static_cast<int>(arm_mode)]);
    printf("cpu state: %s\n", is_thumb? "thumb" : "arm");

    puts("USER & SYSTEM REGS");

    for(int i = 0; i < 16; i++)
    {
        printf("%s: %08x ",user_regs_names[i],user_regs[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }
    }


    puts("\n\nFIQ BANKED");


    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x ",fiq_banked_names[i],fiq_banked[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }        
    }

    puts("\nHI BANKED");

    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x %s: %08x\n",hi_banked_names[i][0],hi_banked[i][0],
            hi_banked_names[i][1],hi_banked[i][1]);
    }

    puts("\nSTAUS BANKED");

    for(int i = 0; i < 5; i++)
    {
        printf("%s: %08x ",status_banked_names[i],status_banked[i]);
        if((i % 2) == 0)
        {
            putchar('\n');
        }       
    }


    printf("\ncpsr: %08x\n",cpsr);

    puts("FLAGS");

    printf("Z: %s\n",is_set(cpsr,Z_BIT)? "true" : "false");
    printf("C: %s\n",is_set(cpsr,C_BIT)? "true" : "false");
    printf("N: %s\n",is_set(cpsr,N_BIT)? "true" : "false");
    printf("V: %s\n",is_set(cpsr,V_BIT)? "true" : "false");

}


// set zero flag based on arg
void Cpu::set_zero_flag(uint32_t v)
{
    cpsr = v == 0? set_bit(cpsr,Z_BIT) : deset_bit(cpsr,Z_BIT); 
}


void Cpu::set_negative_flag(uint32_t v)
{
    cpsr = is_set(v,31)? set_bit(cpsr,N_BIT) : deset_bit(cpsr,N_BIT);
}


// both are set together commonly
// so add a shortcut
void Cpu::set_nz_flag(uint32_t v)
{
    set_zero_flag(v);
    set_negative_flag(v);
}




// set zero flag based on arg
void Cpu::set_zero_flag_long(uint64_t v)
{
    cpsr = v == 0? set_bit(cpsr,Z_BIT) : deset_bit(cpsr,Z_BIT); 
}


void Cpu::set_negative_flag_long(uint64_t v)
{
    cpsr = is_set(v,63)? set_bit(cpsr,N_BIT) : deset_bit(cpsr,N_BIT);   
}


// both are set together commonly
// so add a shortcut
void Cpu::set_nz_flag_long(uint64_t v)
{
    set_zero_flag_long(v);
    set_negative_flag_long(v);
}




void Cpu::switch_mode(cpu_mode new_mode)
{
    // save and load regs
    store_registers(arm_mode);
    load_registers(new_mode);
    arm_mode = new_mode; // finally change modes
    
    // set mode bits in cpsr
    cpsr &= ~0b11111; // mask bottom 5 bits
    cpsr |= get_cpsr_mode_bits(arm_mode);
}


void Cpu::load_registers(cpu_mode mode)
{
    int idx = static_cast<int>(mode);

    switch(mode)
    {

        case cpu_mode::system:
        case cpu_mode::user:
        {
            // load user registers back into registers
            memcpy(regs,user_regs,sizeof(regs));
            break;
        }


        case cpu_mode::fiq:
        {
            // load bottom 8 user regs
            memcpy(regs,user_regs,sizeof(uint32_t)*8);
            regs[PC] = user_regs[PC]; // may be overkill

            // load fiq banked 
            memcpy(&regs[5],fiq_banked,sizeof(uint32_t)*5);
            regs[SP] = hi_banked[idx][0];
            regs[LR] = hi_banked[idx][1];

            break;
        }

        // they all have the same register layout
        // bar the banked ones
        case cpu_mode::supervisor:
        case cpu_mode::abort:
        case cpu_mode::irq:
        case cpu_mode::undefined:
        {
            // load first 13 user regs back to reg
            memcpy(regs,user_regs,sizeof(uint32_t)*13);


            regs[PC] = user_regs[PC]; // may be overkill

            // load hi regs
            regs[SP] = hi_banked[idx][0];
            regs[LR] = hi_banked[idx][1];


            break;          
        }


        default:
        {
            auto err = fmt::format("[load-regs {:08x}]unhandled mode switch: {:x}\n",regs[PC],idx);
            throw std::runtime_error(err);
        }
    }    
}


void Cpu::set_cpsr(uint32_t v)
{
    cpsr = v;

    // confirm this?
    is_thumb = is_set(cpsr,5);
    cpu_mode new_mode = cpu_mode_from_bits(cpsr & 0b11111);
    switch_mode(new_mode);    
}

// store current active registers back into the copies
void Cpu::store_registers(cpu_mode mode)
{
    int idx = static_cast<int>(mode);

    switch(mode)
    {

        case cpu_mode::system:
        case cpu_mode::user:
        {
            // store user registers back into registers
            memcpy(user_regs,regs,sizeof(user_regs));
            break;
        }


        case cpu_mode::fiq:
        {
            // store bottom 8 user regs
            memcpy(user_regs,regs,sizeof(uint32_t)*8);
            user_regs[PC] = regs[PC]; // may be overkill

            // store fiq banked 
            memcpy(fiq_banked,&regs[5],sizeof(uint32_t)*5);
            hi_banked[idx][0] = regs[SP];
            hi_banked[idx][1] = regs[LR];

            break;
        }

        // they all have the same register layout
        // bar the banked ones
        case cpu_mode::supervisor:
        case cpu_mode::abort:
        case cpu_mode::irq:
        case cpu_mode::undefined:
        {
            // write back first 13 regs to user
            memcpy(user_regs,regs,sizeof(uint32_t)*13);
            user_regs[PC] = regs[PC]; // may be overkill

            // store hi regs
            hi_banked[idx][0] = regs[SP];
            hi_banked[idx][1] = regs[LR];

            break;          
        }


        default:
        {
            auto err = fmt::format("[store-regs {:08x}]unhandled mode switch: {:x}\n",regs[PC],idx);
            throw std::runtime_error(err);
        }
    }
}


cpu_mode Cpu::cpu_mode_from_bits(uint32_t v)
{
    switch(v)
    {
        case 0b10000: return cpu_mode::user;
        case 0b10001: return cpu_mode::fiq;
        case 0b10010: return cpu_mode::irq;
        case 0b10011: return cpu_mode::supervisor;
        case 0b10111: return cpu_mode::abort;
        case 0b11011: return cpu_mode::undefined;
        case 0b11111: return cpu_mode::system;
    }

    // clearly no program should attempt this 
    // but is their a defined behavior for it?
    auto err = fmt::format("unknown mode from bits: {:08x}:{:08x}\n",v,regs[PC]);
    throw std::runtime_error(err);
}


// tests if a cond field in an instr has been met
bool Cpu::cond_met(int opcode)
{

    // switch on the cond bits
    // (lower 4)
    switch(static_cast<arm_cond>(opcode & 0xf))
    {
        // z set
        case arm_cond::eq: return is_set(cpsr,Z_BIT);
        
        // z clear
        case arm_cond::ne: return !is_set(cpsr,Z_BIT);

        // c set
        case arm_cond::cs: return is_set(cpsr,C_BIT);

        // c clear
        case arm_cond::cc: return !is_set(cpsr,C_BIT);

        // n set
        case arm_cond::mi: return is_set(cpsr,N_BIT);

        // n clear
        case arm_cond::pl: return !is_set(cpsr,N_BIT);

        // v set
        case arm_cond::vs: return is_set(cpsr,V_BIT); 

        // v clear
        case arm_cond::vc: return !is_set(cpsr,V_BIT);

        // c set and z clear
        case arm_cond::hi: return is_set(cpsr, C_BIT) && !is_set(cpsr,Z_BIT);

        // c clear or z set
        case arm_cond::ls: return !is_set(cpsr,C_BIT) || is_set(cpsr,Z_BIT);

        // n equals v
        case arm_cond::ge: return is_set(cpsr,N_BIT) == is_set(cpsr,V_BIT);

        // n not equal to v
        case arm_cond::lt: return is_set(cpsr,N_BIT) != is_set(cpsr,V_BIT); 

        // z clear and N equals v
        case arm_cond::gt: return !is_set(cpsr,Z_BIT) && is_set(cpsr,N_BIT) == is_set(cpsr,V_BIT);

        // z set or n not equal to v
        case arm_cond::le: return is_set(cpsr,Z_BIT) || is_set(cpsr,N_BIT) != is_set(cpsr,V_BIT);

        // allways
        case arm_cond::al: return true;

    }
    return true; // shoud not be reached
}

// common arithmetic and logical operations


/*
// tests for overflow this happens
// when the sign bit changes when it shouldunt

// causes second test to not show when calc is done
// 2nd operand must be inverted for sub :P
bool did_overflow(uint32_t v1, uint32_t v2, uint32_t ans)
{
    return  is_set((v1 ^ ans) & (v2 ^ ans),31); 
}
^ old code now replaced with compilier builtins
*/

uint32_t Cpu::add(uint32_t v1, uint32_t v2, bool s)
{
    int32_t ans;
    if(s)
    {
        // how to set this shit?
        // happens when a change of sign occurs (so bit 31)
        /// changes to somethign it shouldunt
        bool set_v = __builtin_add_overflow((int32_t)v1,(int32_t)v2,&ans);
        cpsr = ((uint32_t)ans < v1)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT); 

        set_nz_flag(ans);
    }

    else
    {
        ans = v1 + v2;
    }

    return (uint32_t)ans;
}


// needs double checking!
uint32_t Cpu::adc(uint32_t v1, uint32_t v2, bool s)
{

    uint32_t v3 = is_set(cpsr,C_BIT);

    int32_t ans;
    if(s)
    {
        bool set_v = __builtin_add_overflow((int32_t)v1,(int32_t)v2,&ans);
        set_v ^= __builtin_add_overflow((int32_t)ans,(int32_t)v3,&ans);
        cpsr = (uint32_t)ans < (v1+v3)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT); 
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT); 

        set_nz_flag(ans);
    }

    else
    {
        ans = v1 + v2 + v3;
    }

    return (uint32_t)ans;
}


uint32_t Cpu::sub(uint32_t v1, uint32_t v2, bool s)
{
    int32_t ans;
    if(s)
    {
        bool set_v = __builtin_sub_overflow((int32_t)v1,(int32_t)v2,&ans);
        cpsr = (v1 >= v2)? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT);


        set_nz_flag(ans);
    }

    else
    {
        ans = v1 - v2;
    }
    return (uint32_t)ans;
}

// nneds double checking
uint32_t Cpu::sbc(uint32_t v1, uint32_t v2, bool s)
{
    // subtract one from ans if carry is not set
    uint32_t v3 = is_set(cpsr,C_BIT)? 0 : 1;

    int32_t ans;
    if(s)
    {
        bool set_v = __builtin_sub_overflow((int32_t)v1,(int32_t)v2,&ans);
        set_v ^= __builtin_sub_overflow((int32_t)ans,(int32_t)v3,&ans);
        cpsr = (v1 >= (v2+v3))? set_bit(cpsr,C_BIT) : deset_bit(cpsr,C_BIT);
        cpsr = set_v? set_bit(cpsr,V_BIT) : deset_bit(cpsr,V_BIT);


        set_nz_flag(ans);
    }

    else
    {
        ans = v1 - v2 - v3;
    }

    return (uint32_t)ans;
}

uint32_t Cpu::logical_and(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 & v2;

    if(s)
    {
        set_nz_flag(ans);
    }

    return ans;
}

uint32_t Cpu::logical_or(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 | v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::bic(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 & ~v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}

uint32_t Cpu::logical_eor(uint32_t v1, uint32_t v2, bool s)
{
    uint32_t ans = v1 ^ v2;
    if(s)
    {
        set_nz_flag(ans);
    }
    return ans;
}



// write the interrupt req bit
void Cpu::request_interrupt(interrupt i)
{
    cpu_io.interrupt_flag = set_bit(cpu_io.interrupt_flag,static_cast<uint32_t>(i));   
}


void Cpu::do_interrupts()
{
    if(is_set(cpsr,7)) // irqs maksed
    {
        return;
    }

    // the handler will find out what fired for us!
    if((cpu_io.ime & cpu_io.interrupt_enable & cpu_io.interrupt_flag) != 0)
    {
        //printf("interrupt fired!");
        service_interrupt();
    }
}

// do we need to indicate the interrupt somewhere?
// or does the handler check if?
void Cpu::service_interrupt()
{
    int idx = static_cast<int>(cpu_mode::irq);

    // spsr for irq = cpsr
    status_banked[idx] = cpsr;

    // lr is next instr + 4 for an irq 
    hi_banked[idx][1] = regs[PC] + 4;

    // irq mode switch
    switch_mode(cpu_mode::irq);

    
    // switch to arm mode
    is_thumb = false; // switch to arm mode
    cpsr = deset_bit(cpsr,5); // toggle thumb in cpsr
    cpsr = set_bit(cpsr,7); //set the irq bit to mask interrupts

    write_log("[irq {:08x}] interrupt flag: {:02x} ",regs[PC],cpu_io.interrupt_flag);

    regs[PC] = 0x18; // irq handler    
}


// check if for each dma if any of the start timing conds have been met
// should store all the dma information in struct so its nice to access
// also find out when dmas are actually processed?

// ignore dma till after armwrestler we are probably gonna redo the api for the most part anyways
void Cpu::handle_dma(dma_type req_type, int special_dma)
{

    UNUSED(special_dma);
    UNUSED(req_type);

    if(dma_in_progress) 
    { 
        return; 
    }

/*
    static constexpr uint32_t zero_table[4] = {0x4000,0x4000,0x4000,0x10000};
    for(int i = 0; i < 4; i++)
    {

        uint32_t cnt_addr = IO_DMA0CNT_H+i*12;
        uint16_t dma_cnt = mem->handle_read<uint16_t>(mem->io,cnt_addr);

        if(is_set(dma_cnt,15)) // dma is enabled
        {
            auto type = static_cast<dma_type>((dma_cnt >> 12) & 0x3);


            bool is_triggered = false;

            // speical dma modes on trigger for their respective dma modes
            if(type == dma_type::special)
            {
                if(i == special_dma)
                {
                    is_triggered = true;
                }
            }

            else
            {
                is_triggered = true;
            }

            if(is_triggered)
            {
                uint32_t word_count_addr = IO_DMA0CNT_L + i * 12;


                if(is_set(dma_cnt,9)) // repeat bit so reload word count
                {
                    dma_regs[i].nn = mem->handle_read<uint16_t>(mem->io,word_count_addr);
                }

                 // if a zero len transfer it uses the max len for that dma
                if(dma_regs[i].nn == 0)
                {
                    dma_regs[i].nn = zero_table[i];
                }



                do_dma(dma_cnt,req_type,i);
                mem->handle_write<uint16_t>(mem->io,cnt_addr,dma_cnt); // write back the control reg!
            }
        }
    }
*/
}


// what kind of side affects can we have here!?
// need to handle an n of 0 above in the calle
// also need to handle repeats!

// this needs to know the dma number aswell as the type of dma
// <-- how does gamepak dma work
// this seriously needs a refeactor
void Cpu::do_dma(uint16_t &dma_cnt,dma_type req_type, int dma_number)
{

    UNUSED(req_type);
    UNUSED(dma_number);

    if(is_set(dma_cnt,11))
    {
        throw std::runtime_error("[unimplemented] gamepak dma!");
    }

/*
    dma_in_progress = true;

    Dma_reg &dma_reg = dma_regs[dma_number];

    uint32_t source = dma_reg.src;
    uint32_t dest = dma_reg.dst;


    bool is_half = !is_set(dma_cnt,10);
    uint32_t size = is_half? ARM_HALF_SIZE : ARM_WORD_SIZE;

    source &= 0x0fffffff;
    dest &= 0x0fffffff;

    for(size_t i = 0; i < dma_reg.nn; i++)
    {
        uint32_t offset = i * size;

        if(is_half)
        {
            uint16_t v = mem->read_memt<uint16_t>(source+offset);
            mem->write_memt<uint16_t>(dest+offset,v);
        }

        else
        {
            uint32_t v = mem->read_memt<uint32_t>(source+offset);
            mem->write_memt<uint32_t>(dest+offset,v);
        }
    }

    static constexpr interrupt dma_interrupt[4] = {interrupt::dma0,interrupt::dma1,interrupt::dma2,interrupt::dma3}; 
    if(is_set(dma_cnt,14)) // do irq on finish
    {
        request_interrupt(dma_interrupt[dma_number]);
    }


    if(!is_set(dma_cnt,9) || req_type == dma_type::immediate || is_set(dma_cnt,11) ) // dma does not repeat
    {
        dma_cnt = deset_bit(dma_cnt,15); // disable it
    }

    int sad_mode = (dma_cnt >> 8) & 3;
    int dad_mode = (dma_cnt >> 6) & 3;


    switch(sad_mode)
    {
        case 0: // increment
        {
            dma_reg.src += dma_reg.nn * size;
            break;
        }

        case 1: // decrement
        {
            dma_reg.src -= dma_reg.nn * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        case 3: // invalid
        {
            throw std::runtime_error("sad mode of 3!");
        }
    }


    switch(dad_mode)
    {
        case 0: // increment
        {
            dma_reg.dst += dma_reg.nn * size;
            break;
        }

        case 1: // decrement
        {
            dma_reg.dst -= dma_reg.nn * size;
            break;
        }

        case 2: // fixed (do nothing)
        {
            break;
        }

        case 3: // incremnt + reload
        {
            uint32_t dad_addr = IO_DMA0DAD + dma_number * 12;
            dma_reg.dst = mem->handle_read<uint32_t>(mem->io,dad_addr);
            dma_reg.dst += dma_reg.nn * size;
            break;
        }
    }
*/

    dma_in_progress = false;
}

}