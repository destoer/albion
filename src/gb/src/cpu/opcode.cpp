#include <gb/cpu.h>
#include <gb/memory.h>
#include <gb/opcode_table.h>
#include <gb/cpu.inl>
#include <albion/debug.h>



namespace gameboy
{


void Cpu::exec_instr_debug()
{
	const auto x = mem.read_mem(pc);
	if(debug.breakpoint_hit(pc,x,break_type::execute))
	{
		// halt until told otherwhise :)
		debug.print_console("execute breakpoint hit ({:x}:{:x})\n",pc,x);
		debug.halt();
		return;
	}
	exec_instr_no_debug();
}

template<const int REG>
void Cpu::write_r16_group1(u16 v)
{
	static_assert(REG <= 3,"register not valid for group");

	if constexpr(REG == 0) 
	{
		bc = v;
	}

	else if constexpr(REG == 1) 
	{
		de = v;
	}

	else if constexpr(REG == 2)
	{
		hl = v;
	}

	else if constexpr(REG == 3)
	{
		sp = v;
	}
}

template<const int REG>
void Cpu::write_r8(u8 v)
{
	static_assert(REG <= 7);
	if constexpr(REG == 0)
	{
		write_upper(&bc,v);
	}

	else if constexpr(REG == 1)
	{
		write_lower(&bc,v);
	}

	else if constexpr(REG == 2)
	{
		write_upper(&de,v);
	}

	else if constexpr(REG == 3)
	{
		write_lower(&de,v);
	}

	else if constexpr(REG == 4)
	{
		write_upper(&hl,v);
	}

	else if constexpr(REG == 5)
	{
		write_lower(&hl,v);
	}

	else if constexpr(REG == 6)
	{
		mem.write_memt(hl,v);
	}

	else if constexpr(REG == 7)
	{
		a = v;
	}	
}

template<const int REG>
u8 Cpu::read_r8()
{
	static_assert(REG <= 7);
	if constexpr(REG == 0)
	{
		return read_b();
	}

	else if constexpr(REG == 1)
	{
		return read_c();
	}

	else if constexpr(REG == 2)
	{
		return read_d();
	}

	else if constexpr(REG == 3)
	{
		return read_e();
	}

	else if constexpr(REG == 4)
	{
		return read_h();
	}

	else if constexpr(REG == 5)
	{
		return read_l();
	}

	else if constexpr(REG == 6)
	{
		return mem.read_memt(hl);
	}

	else if constexpr(REG == 7)
	{
		return a;
	}	
}

template<const int REG>
u16 Cpu::read_r16_group3()
{
	static_assert(REG <= 3);

	if constexpr(REG == 0)
	{
		return bc;
	}

	else if constexpr(REG == 1)
	{
		return de;
	}

	else if constexpr(REG == 2)
	{
		return hl;
	}

	else if constexpr(REG == 3)
	{
		return read_af();
	}	
}


template<const int REG>
u16 Cpu::read_r16_group1()
{
	static_assert(REG <= 3,"register not valid for group");

	if constexpr(REG == 0) 
	{
		return bc;
	}

	else if constexpr(REG == 1) 
	{
		return de;
	}

	else if constexpr(REG == 2)
	{
		return hl;
	}

	else if constexpr(REG == 3)
	{
		return sp;
	}
}


template<const int REG>
void Cpu::write_r16_group3(u16 v)
{
	static_assert(REG <= 3);

	if constexpr(REG == 0)
	{
		bc = v;
	}

	else if constexpr(REG == 1)
	{
		de= v;
	}

	else if constexpr(REG == 2)
	{
		hl = v;
	}

	else if constexpr(REG == 3)
	{
		write_af(v);
	}	
}

template<const int REG>
void Cpu::write_r16_group2(u16 v)
{
	static_assert(REG <= 3);

	if constexpr(REG == 0)
	{
		bc = v;
	}

	else if constexpr(REG == 1)
	{
		de = v;
	}

	else if constexpr(REG == 2)
	{
		hl = v;
	}

	else if constexpr(REG == 3)
	{
		hl = v;
	}		
}

template<const int REG>
u16 Cpu::read_r16_group2()
{
	static_assert(REG <= 3);

	if constexpr(REG == 0)
	{
		return bc;
	}

	else if constexpr(REG == 1)
	{
		return de;
	}

	else if constexpr(REG == 2)
	{
		return hl;
	}

	else if constexpr(REG == 3)
	{
		return hl;
	}		
}

template<const int COND>
bool Cpu::cond()
{
	static_assert(COND <= 3);
	// nz
	if constexpr(COND == 0)
	{
		return !zero;
	}

	// z
	else if constexpr(COND == 1)
	{
		return zero;
	}

	// nc
	else if constexpr(COND == 2)
	{
		return !carry;
	}

	// c
	else if constexpr(COND == 3)
	{
		return carry;
	}
}

void Cpu::undefined_opcode()
{
	const auto str = fmt::format("[ERROR] invalid opcode {:x} at {:x}:{}",mem.read_mem(pc-1),pc-1,disass.disass_op(pc-1));
	spdlog::error("{}",str);
	throw std::runtime_error(str);		
}

void Cpu::undefined_opcode_cb()
{
	const auto str = fmt::format("[ERROR] invalid cb opcode {:x} at {:x}:{}",mem.read_mem(pc-1),pc-2,disass.disass_op(pc-2));
	spdlog::error("{}",str);
	throw std::runtime_error(str);		
}

void Cpu::nop() 
{

}

void Cpu::jp()
{
	const u16 source = pc-1;
	pc = mem.read_wordt(pc);
	cycle_tick_t(4); // internal
	debug.trace.add(source,pc);	
}

void Cpu::ld_u16_sp()
{
	mem.write_wordt(mem.read_wordt(pc),sp);
	pc += 2; // for two immediate ops	
}

template<const int REG>
void Cpu::ld_r16_u16()
{
	write_r16_group1<REG>(mem.read_wordt(pc));
	pc += 2;
}

void Cpu::ld_u16_a()
{
	mem.write_memt(mem.read_wordt(pc),a);
	pc += 2;	
}

template<const int REG>
void Cpu::ld_r8_u8()
{
	write_r8<REG>(mem.read_memt(pc++));
}


void Cpu::ld_ffu8_a()
{
	mem.write_iot((0xff00+mem.read_memt(pc++)),a);
}

void Cpu::call()
{
	const u16 source = pc-1;
	u16 v = mem.read_wordt(pc);
	pc += 2;
	cycle_tick_t(4); // internal
	write_stackwt(pc);
	pc = v;
	debug.trace.add(source,pc);	
}

void Cpu::halt()
{
	handle_halt();
}

template<const int DST,const int SRC>
void Cpu::ld_r8_r8()
{
	// halt
	static_assert(!(DST == 6 && SRC == 6));
	write_r8<DST>(read_r8<SRC>());
}

void Cpu::jr()
{
	const auto operand = static_cast<int8_t>(mem.read_memt(pc++));
	cycle_tick_t(4); // internal delay
	pc += operand;		
}

void Cpu::ret()
{
	const u16 source = pc-1;
	pc = read_stackwt();	
	cycle_tick_t(4); // internal
	debug.trace.add(source,pc);	
}

void Cpu::di()
{
	// di should disable immediately unlike ei!
	// if we havent just exected a ei then we are done and can reset the state
	// else we need to mark it so ei wont reneable it by mistake
	instr_side_effect = instr_side_effect == instr_state::ei? instr_state::di : instr_state::normal;
	interrupt_enable = false; 
	update_intr_fire();	
}

template<const int REG>
void Cpu::push()
{
	const u16 reg = read_r16_group3<REG>();
	cycle_tick_t(4); // internal
	write_stackwt(reg);
}

template<const int REG>
void Cpu::pop()
{
	write_r16_group3<REG>(read_stackwt());
}

template<const int REG>
void Cpu::dec_r16()
{
	const u16 reg = read_r16_group1<REG>();
	oam_bug_write(reg);
	cycle_tick_t(4); // internal
	write_r16_group1<REG>(reg-1);		
}

template<const int REG>
void Cpu::inc_r16()
{
	const u16 reg = read_r16_group1<REG>();
	oam_bug_write(reg);
	cycle_tick_t(4); // internal
	write_r16_group1<REG>(reg+1);	
}


// how do we want to handle specializing this 
// for ldi and ldd?
// need to impl group2
template<const int REG>
void Cpu::ld_a_r16()
{
	const u16 reg = read_r16_group2<REG>();
	a = mem.read_memt(reg);

	// ldi
	if constexpr(REG == 2)
	{
		write_r16_group2<REG>(reg+1);
	}

	// ldd 
	else if constexpr(REG == 3)
	{
		write_r16_group2<REG>(reg-1);
	}
}

void Cpu::set_zero(u8 v)
{
	zero = !v;
}

void Cpu::instr_or(u8 v)
{
	a |= v;
	// reset flags
	negative = false;
	half = false;
	carry = false;
	set_zero(a);	
}

template<const int REG>
void Cpu::or_r8()
{
	const u8 v = read_r8<REG>();
	instr_or(v);
}

template<const int COND>
void Cpu::jr_cond()
{
	const auto operand = static_cast<int8_t>(mem.read_memt(pc++));
	if(cond<COND>())
	{
		cycle_tick_t(4); // internal delay
		pc += operand;
	}		
}

void Cpu::ld_a_ffu8()
{
	a = mem.read_iot(0xff00+mem.read_memt(pc++));
}

void Cpu::instr_cp(u8 v)
{

	negative = true;
	
	zero = a == v;

	// check half carry
	half = (((a & 0x0f) - (v & 0x0f)) < 0);


	carry = v > a; 
}

template<const int REG>
void Cpu::cp_r8()
{
	const u8 v = read_r8<REG>();
	instr_cp(v);
}

void Cpu::cp_u8()
{
	instr_cp(mem.read_memt(pc++));
}

void Cpu::or_u8()
{
	instr_or(mem.read_memt(pc++));
}

void Cpu::ld_a_u16()
{
	a = mem.read_memt(mem.read_wordt(pc));
	pc += 2;	
}


void Cpu::instr_and(u8 v)
{
	// set only the half carry flag
	half = true;
	carry = false;
	negative = false;

	// set if result is zero 
	a &= v;
	set_zero(a);	
}

void Cpu::and_u8()
{
	instr_and(mem.read_memt(pc++));
}

template<const int REG>
void Cpu::and_r8()
{
	instr_and(read_r8<REG>());
}

template<const int COND>
void Cpu::call_cond()
{
	const u16 source = pc-1;
	const auto v = mem.read_wordt(pc);
	pc += 2;
	if(cond<COND>())
	{
		cycle_tick_t(4);  // internal delay
		write_stackwt(pc);
		pc = v;
		debug.trace.add(source,pc);
	}	
}

template<const int REG>
void Cpu::dec_r8()
{
	u8 reg = read_r8<REG>();
    reg -= 1;

    // the N flag
	negative = true;

    set_zero(reg);

	// check the carry 
	half = is_set(((reg+1)&0xf)-1,4);
	write_r8<REG>(reg);
}

template<const int REG>
void Cpu::inc_r8()
{
	u8 reg = read_r8<REG>();

	// deset negative
	negative = false;


	reg += 1;

	// if there is a carry from bit 3 we will now be zero
	// or atleast one
	half = (reg & 0xf) == 0;
	
    set_zero(reg);
	write_r8<REG>(reg);
}

void Cpu::instr_xor(u8 v)
{
	// reset flags
	negative = false;
	half = false;
	carry = false;

	a ^= v;
	set_zero(a);	
}

template<const int REG>
void Cpu::xor_r8()
{
	const u8 reg = read_r8<REG>();
	instr_xor(reg);
}

void Cpu::xor_u8()
{
	instr_xor(mem.read_memt(pc++));
}

template<const int REG>
void Cpu::ld_r16_a()
{
	const u16 reg = read_r16_group2<REG>();
	mem.write_memt(reg,a);

	// ldi
	if constexpr(REG == 2)
	{
		write_r16_group2<REG>(reg+1);
	}

	// ldd 
	else if constexpr(REG == 3)
	{
		write_r16_group2<REG>(reg-1);
	}
}

void Cpu::instr_add(u8 v)
{
	// deset negative
	negative = false;


	// test carry from bit 3
	// set the half carry if there is
	half = (is_set((a & 0x0f) + (v & 0x0f),4));

	
	// check carry from bit 7
	carry = (a + v > 255);
		
	a += v;
	set_zero(a);	
}

template<const int REG>
void Cpu::add_r8()
{
	const u8 reg = read_r8<REG>();
	instr_add(reg);
}

void Cpu::add_u8()
{
	instr_add(mem.read_memt(pc++));
}


void Cpu::instr_sub(u8 v)
{
	// set negative
	negative = true;
	
	zero = a == v;


	// check half carry
	half = (((a & 0x0f) - (v & 0x0f)) < 0);

	carry = v > a;

	a -= v;
}

template<const int REG>
void Cpu::sub_r8()
{
	const u8 reg = read_r8<REG>();
	instr_sub(reg);
}

void Cpu::sub_u8()
{
	instr_sub(mem.read_memt(pc++));
}


// n + carry flag to a
void Cpu::instr_adc(u8 v)
{
	const u8 reg = a;
	
	const int carry_val = carry ? 1 : 0;
	
	const int result = reg + v + carry_val;
	
	// deset negative
	negative = false;
	
	carry = (result > 0xff);

	
	half = (is_set((reg & 0x0f) + (v & 0x0f) + carry_val,4));
		
	a = result;
	set_zero(a);
}

template<const int REG>
void Cpu::adc_r8()
{
	const u8 reg = read_r8<REG>();
	instr_adc(reg);	
}

void Cpu::adc_u8()
{
	instr_adc(mem.read_memt(pc++));
}

template<const int COND>
void Cpu::ret_cond()
{
	const u16 source = pc-1;
	cycle_tick_t(4); // internal
	if(cond<COND>())
	{
		pc = read_stackwt();
		cycle_tick_t(4);  // internal
		debug.trace.add(source,pc);
	}		
}

template<const int REG>
void Cpu::add_hl_r16()
{
	u16 dst = hl;
	const u16 oper = read_r16_group1<REG>();

	// deset negative
	negative = false;

	// check for carry from bit 11
	half = (is_set((dst & 0x0fff) + (oper & 0x0fff),12));
	
	// check for full carry 
	carry = (is_set(dst + oper,16));

	dst += oper;
	
	hl = dst;
	cycle_tick_t(4); // internal
}

void Cpu::jp_hl()
{
	const u16 source = pc-1;
	pc = hl;
	debug.trace.add(source,pc);	
}

template<const int COND>
void Cpu::jp_cond()
{
	const u16 source = pc-1;
	const auto v =  mem.read_wordt(pc);
	pc += 2;
	if(cond<COND>())
	{
		pc = v;
		cycle_tick_t(4); // internal delay
		debug.trace.add(source,pc);
	}		
}

void Cpu::ld_hl_sp_i8()
{
	hl = instr_addi(static_cast<int8_t>(mem.read_memt(pc++)));
	cycle_tick_t(4); // internal	
}


// for the sp add opcodes
u16 Cpu::instr_addi(int8_t v)
{
	// deset negative & zero
	negative = false;
	zero = false;

	// test carry from bit 3
	// set the half carry if there is
	half = (is_set((sp & 0x0f) + (v & 0x0f),4));
	
	carry = (is_set((sp & 0xff) + (v & 0xff),8));

	
	return sp + v;	
}

// bcd
void Cpu::daa()
{
	//https://forums.nesdev.com/viewtopic.php?f=20&t=15944
	if (!negative) 
	{  
		// after an addition, adjust if (half-)carry occurred or if result is out of bounds
		if (carry || a > 0x99) 
		{ 
			a += 0x60; 
			carry = true;
		}
		if (half || (a & 0x0f) > 0x09)  
		{ 
			a += 0x6; 
		}
	} 
	
	else 
	{  
		// after a subtraction, only adjust if (half-)carry occurred
		if (carry) 
		{
			a -= 0x60; 
		}
		
		if (half) 
		{ 
			a -= 0x6; 
		}
	}
	
	// preserve C and N flags
	//f &= (1 << C) | (1 << N);
	half = false;

	set_zero(a);
}

void Cpu::ld_sp_hl()
{
	sp = hl;
	cycle_tick_t(4); // internal
}

void Cpu::ei()
{
	// if we execute two ie in a row we dont need to bother
	// as we are allready enabled
	if(instr_side_effect != instr_state::ei)
	{
		// caller will check opcode and handle it
		instr_side_effect = instr_state::ei;

		exec_instr(); 
	}

	// if last instr was a di we should not enable
	if(instr_side_effect != instr_state::di)
	{
		interrupt_enable = true;
	}

	instr_side_effect = instr_state::normal;

	update_intr_fire();
}

void Cpu::stop()
{
	pc += 1; // skip over next byte
			
	if(is_cgb && is_set(mem.io[IO_SPEED],0))
	{
		mem.io[IO_SPEED] = deset_bit(mem.io[IO_SPEED],0); // clear the bit
		
		switch_double_speed();
		
		if(is_double)
		{
			mem.io[IO_SPEED] = set_bit(mem.io[IO_SPEED],7);
		}
	
		else // single speed 
		{
			mem.io[IO_SPEED] = deset_bit(mem.io[IO_SPEED],7);
		}
	}
	
	else // almost nothing triggers this 
	{
		spdlog::warn("[WARNING] stop opcode hit at {:x}",pc);
	}
}

void Cpu::add_sp_i8()
{
	sp = instr_addi(static_cast<int8_t>(mem.read_memt(pc++)));
	cycle_tick_t(8); // internal delay (unsure)	
}

void Cpu::instr_sbc(u8 v)
{
	const u8 reg = a;

	const int carry_val = carry;
	
	const int result = reg - v - carry_val;
	
	// set negative
	negative = true;
	
	carry = (result < 0);

	
	half = ((reg & 0x0f) - (v & 0x0f) - carry_val < 0);

	a = result;
	set_zero(a);	
}

template<const int REG>
void Cpu::sbc_r8()
{
	const u8 reg = read_r8<REG>();
	instr_sbc(reg);
}

void Cpu::sbc_u8()
{
	instr_sbc(mem.read_memt(pc++));
}

void Cpu::reti()
{
	const u16 source = pc-1;
	pc = read_stackwt();	
	cycle_tick_t(4);// internal
	interrupt_enable = true; // re-enable interrupts
	update_intr_fire();
	debug.trace.add(source,pc);
}

template<const int ADDR, const int OP>
void Cpu::rst()
{
	const u16 source = pc-1;
	if(mem.read_mem(ADDR) == OP)
	{
		// if oam dma is active then we there is a chance this wont loop
		if(!mem.oam_dma_active)
		{
			spdlog::error("rst infinite loop at {:x}->{:x}",pc,ADDR);
			throw std::runtime_error("infinite rst lockup");
		}
	}
	cycle_tick_t(4); // internal
	write_stackwt(pc);
	pc = ADDR;
	debug.trace.add(source,pc);	
}

void Cpu::ld_a_ff00_c()
{
	a = mem.read_iot(0xff00 + read_lower(bc));
}

void Cpu::ld_ff00_c_a()
{
	mem.write_iot(0xff00 + read_lower(bc),a);
}

void Cpu::cpl()
{
	// cpl (flip bits in a)
	// set H and N
	half = true;
	negative = true;
	a = ~a;	
}

void Cpu::scf()
{
	// set the carry flag deset h and N
	carry = true;
	negative = false;
	half = false;	
}

void Cpu::ccf()
{
	carry = !carry;
	negative = false;
	half = false;	
}

void Cpu::exec_instr_no_debug()
{
    const auto opcode = fetch_opcode();
	std::invoke(opcode_table[opcode],this);
}





void Cpu::cb_opcode()
{
	const u8 cbop = mem.read_memt(pc++);
	std::invoke(cb_table[cbop],this);
}

template<const int REG>
void Cpu::srl()
{
	u8 reg = read_r8<REG>();
	half = false;
	negative = false;

	carry = is_set(reg,0);

	
	reg >>= 1;

	set_zero(reg);
	
	write_r8<REG>(reg);
}

u8 Cpu::instr_rrc(u8 v)
{
	carry = is_set(v,0);
	
	negative = false;
	half = false;

	v >>= 1;
	
	v |= carry << 7;
	set_zero(v);
	
	return v;
}

void Cpu::rrca()
{
	a = instr_rrc(a);
	zero = false;
}


template<const int REG>
void Cpu::rrc_r8()
{
	write_r8<REG>(instr_rrc(read_r8<REG>()));
}

u8 Cpu::instr_rr(u8 v)
{
	const bool set = is_set(v,0);
	
	v >>= 1;
	
	// bit 7 gets carry 
	v |= carry << 7;

	// deset negative
	negative = false;
	// unset half
	half = false;
	
	// carry gets bit 0
	carry = set;

	set_zero(v);
	
	return v;
}

template<const int REG>
void Cpu::rr_r8()
{
	write_r8<REG>(instr_rr(read_r8<REG>()));
}

void Cpu::rra()
{
	a = instr_rr(a);
	zero = false;
}


u8 Cpu::instr_rlc(u8 v)
{
	carry = is_set(v,7);
		
	v <<= 1;
	
	negative = false;
	half = false;
	
	v |= carry;

	set_zero(v);
	return v;
}

void Cpu::rlca()
{
	a = instr_rlc(a);
	zero = false;	
}

template<const int REG>
void Cpu::rlc_r8()
{
	write_r8<REG>(instr_rlc(read_r8<REG>()));
}


// swap upper and lower nibbles 
template<const int REG>
void Cpu::instr_swap()
{
	const u8 reg = read_r8<REG>();

	// reset flags
	negative = false;
	half = false;
	carry = false;

    set_zero(reg);

	write_r8<REG>(((reg & 0x0f) << 4 | (reg & 0xf0) >> 4));	
}

u8 Cpu::instr_rl(u8 v)
{
	const bool cond = is_set(v,7); // cache if 7 bit is set
	
	// perform the rotation
	// shift the register left
	v <<= 1;
	
	// bit 0 gets bit of carry flag
	v |= carry;
	
	// deset half carry 
	half = false;
	negative = false;
	
	// Carry flag gets bit 7 of reg
	carry = cond;


	set_zero(v);
	
	return v;	
}

template<const int REG>
void Cpu::rl_r8()
{
	write_r8<REG>(instr_rl(read_r8<REG>()));
}

void Cpu::rla()
{
	a = instr_rl(a);
	zero = false;
}

template<const int REG>
void Cpu::sla_r8()
{
	u8 reg = read_r8<REG>();
	// reset flags
	half = false;
	negative = false;

	carry = is_set(reg,7); // cache if 7 bit is set

	reg <<= 1;
	
	set_zero(reg);

	write_r8<REG>(reg);
}

template<const int REG>
void Cpu::sra_r8()
{
	u8 reg = read_r8<REG>();
	negative = false;
	half = false;
	
	carry = is_set(reg,0);
	const bool set = is_set(reg,7);
	
	reg >>= 1;
	
	reg |= set << 7;

	set_zero(reg);
	
	write_r8<REG>(reg);
}


template<const int REG, const int BIT>
void Cpu::bit_r8()
{
	// unuset negative
	negative = false;

	zero = !is_set(read_r8<REG>(),BIT);
	
	// set half carry
	half = true;		
}

template<const int REG,const int BIT>
void Cpu::res_r8()
{
	write_r8<REG>(deset_bit(read_r8<REG>(),BIT));
}

template<const int REG,const int BIT>
void Cpu::set_r8()
{
	write_r8<REG>(set_bit(read_r8<REG>(),BIT));
}

}