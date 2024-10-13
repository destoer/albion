#include <n64/n64.h>

namespace nintendo64
{

void write_tlb(N64& n64, u32 idx) 
{
    auto& cop0 = n64.cpu.cop0;
    auto& tlb = n64.mem.tlb;
    
    tlb.entry[idx].page_mask = cop0.page_mask;
    tlb.entry[idx].entry_hi = cop0.entry_hi;
    tlb.entry[idx].entry_hi.vpn2 = tlb.entry[idx].entry_hi.vpn2 & ~tlb.entry[idx].page_mask;
    
    tlb.entry[idx].entry_lo_one = cop0.entry_lo_one;
    tlb.entry[idx].entry_lo_zero = cop0.entry_lo_zero;

    const bool g = cop0.entry_lo_one.g && cop0.entry_lo_zero.g;

    tlb.entry[idx].entry_lo_one.g = g;
    tlb.entry[idx].entry_lo_zero.g = g;
}


// tlb instrs
// TODO: i think we can saftely ignore these until we actually try to access a tlb section?
void instr_tlbwi(N64& n64, const Opcode &opcode)
{
    UNUSED(opcode);
    auto& cop0 = n64.cpu.cop0;
    write_tlb(n64,cop0.index.idx);
}

void instr_tlbp(N64& n64, const Opcode &opcode)
{
    UNUSED(opcode);
    auto& tlb = n64.mem.tlb;
    auto &cop0 = n64.cpu.cop0;

    for(u32 e = 0; e < TLB_SIZE; e++) 
    {
        auto& entry = tlb.entry[e];

        // scan for a matching page number with, either a matching asid or ignore
        if((entry.entry_hi.vpn2 == cop0.entry_hi.vpn2) && 
            ((entry.entry_lo_zero.g)|| entry.entry_hi.asid == cop0.entry_hi.asid))
        {
            cop0.index.idx = e;
            return;
        }
    }

    cop0.index.p = true;
}

void instr_tlbr(N64& n64, const Opcode &opcode)
{
    UNUSED(opcode);

    auto& tlb = n64.mem.tlb;
    auto &cop0 = n64.cpu.cop0;

    auto& entry = tlb.entry[cop0.index.idx];

    cop0.page_mask = entry.page_mask;
    cop0.entry_hi = entry.entry_hi;
    cop0.entry_lo_one = entry.entry_lo_one;
    cop0.entry_lo_zero = entry.entry_lo_zero;
}

}