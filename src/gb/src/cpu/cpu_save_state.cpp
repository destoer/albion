#include <gb/cpu.h>

namespace gameboy
{

void Cpu::save_state(std::ofstream &fp)
{
    file_write_var(fp,internal_timer);
    file_write_var(fp,joypad_state);
    file_write_var(fp,a);
    file_write_var(fp,carry);
    file_write_var(fp,half);
    file_write_var(fp,negative);
    file_write_var(fp,zero);
    file_write_var(fp,bc);
    file_write_var(fp,de);
    file_write_var(fp,hl);
    file_write_var(fp,sp);
    file_write_var(fp,pc);
    file_write_var(fp,instr_side_effect);
    file_write_var(fp,interrupt_enable);
    file_write_var(fp,is_cgb);
    file_write_var(fp,is_double);
    file_write_var(fp,serial_cyc);
    file_write_var(fp,serial_cnt);
    file_write_var(fp,interrupt_req);
    file_write_var(fp,interrupt_fire);
    file_write_var(fp,is_sgb);
}


dtr_res Cpu::load_state(std::ifstream &fp)
{
    dtr_res err = file_read_var(fp,internal_timer);
    err |= file_read_var(fp,joypad_state);
    err |= file_read_var(fp,a);
    err |= file_read_var(fp,carry);
    err |= file_read_var(fp,half);
    err |= file_read_var(fp,negative);
    err |= file_read_var(fp,zero);
    err |= file_read_var(fp,bc);
    err |= file_read_var(fp,de);
    err |= file_read_var(fp,hl);
    err |= file_read_var(fp,sp);
    err |= file_read_var(fp,pc);
    err |= file_read_var(fp,instr_side_effect);
    if(instr_side_effect > instr_state::di)
    {
        spdlog::error("load_state invalid instr state");
        return dtr_res::err;
    }
    err |= file_read_var(fp,interrupt_enable);
    err |= file_read_var(fp,is_cgb);
    err |= file_read_var(fp,is_double);
    err |= file_read_var(fp,serial_cyc);
    err |= file_read_var(fp,serial_cnt);
    err |= file_read_var(fp,interrupt_req);
    err |= file_read_var(fp,interrupt_fire);
    err |= file_read_var(fp,is_sgb);
    
    return err;
}

}