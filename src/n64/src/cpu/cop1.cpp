#include <fenv.h>

namespace nintendo64
{

b32 cop1_usable(N64& n64)
{
    auto& status = n64.cpu.cop0.status;

    if(!status.cu1)
    {
        coprocessor_unusable(n64,1);
        return false;
    }

    return true;
}

void check_cop1_exception(N64& n64)
{
    auto& cop1 = n64.cpu.cop1;

    if((cop1.enable & cop1.cause) || is_set(cop1.cause,5))
    {
        standard_exception(n64,beyond_all_repair::FLOATING_POINT_EXCEPTION);
    }
}


void write_cop1_control(N64& n64, u32 idx, u32 v)
{ 
    auto& cop1 = n64.cpu.cop1;


    // only the control reg is writeable
    if(idx == 31)
    {
        cop1.fs = is_set(v,24);
        cop1.c = is_set(v,23);

        cop1.cause = (v >> 12) & 0b111'111;

        cop1.enable = (v >> 7) & 0b111'11;
        cop1.flags = (v >> 2) & 0b111'11;
        cop1.rounding = v & 0b11;

        // use fe to set the correct round mode
        switch(cop1.rounding)
        {
            case 0:
            {
                fesetround(FE_TONEAREST);
                break;
            }

            case 1:
            {
                fesetround(FE_TOWARDZERO);
                break;
            }

            case 2:
            {
                fesetround(FE_UPWARD);
                break;
            }

            case 3:
            {
                fesetround(FE_DOWNWARD);
                break;
            }
        }


        check_cop1_exception(n64);
    }
}

u32 read_cop1_control(N64& n64, u32 idx)
{
    auto& cop1 = n64.cpu.cop1;

    switch(idx)
    {
        case 31:
        { 
            return (cop1.fs << 24) | (cop1.c << 23) | (cop1.cause << 12) | 
            (cop1.enable << 7) | (cop1.flags << 2) | cop1.rounding;
        }

        // revision register
        case 0: 
        {
            return 0xa00;
        }

        default: return 0;
    }
}

u32 calc_float_reg_idx(N64& n64, u32 reg)
{
    auto& status = n64.cpu.cop0.status;
    return reg >> (!status.fr);
}

f64 read_cop1_reg(N64& n64, u32 reg)
{
    const u32 idx = calc_float_reg_idx(n64,reg);

    return n64.cpu.cop1.regs[idx];
}

void write_cop1_reg(N64& n64, u32 reg, f64 v)
{
    const u32 idx = calc_float_reg_idx(n64,reg);

    n64.cpu.cop1.regs[idx] = v;
}


}