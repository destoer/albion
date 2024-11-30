#include <n64/n64.h>

namespace nintendo64
{

void bad_vaddr_exception(N64& n64, u64 address, u32 code);

std::optional<u64> translate_vaddr(N64& n64, u64 addr, bool write) {
    const u16 tlb_set = 0b11'11'00'00'11111111;
    const u32 idx = (addr & 0xf000'0000) >> 28;

    // TLB mapped
    if(is_set(tlb_set,idx))
    {
        auto& cop0 = n64.cpu.cop0;
        auto& tlb = n64.mem.tlb;

        // Scan for a match in the tlb
        for(u32 e = 0; e < TLB_SIZE; e++) 
        {
            auto& entry = tlb.entry[e];

            // Not sure how the valid checks work or for that matter dirty bits which low entry is used
            if(!(entry.entry_lo_zero.v && entry.entry_lo_one.v))
            {
                continue;
            }

            // Page mask defines which bits are used of the vaddr on top of the base 12
            const u64 page_mask = 0xfff | entry.page_mask << 12;
            const auto page_offset = addr & page_mask;
            const auto vpn2 = addr & ~page_mask;

            if((entry.entry_hi.vpn2 & ~page_mask) == vpn2 && (cop0.entry_hi.asid == entry.entry_hi.asid || entry.entry_lo_zero.g)) 
            {   
                // Attempt to write to a read only entry!
                if(!(entry.entry_lo_zero.d && entry.entry_lo_one.d) && write)
                {
                    bad_vaddr_exception(n64,addr,beyond_all_repair::TLBM);
                    return std::nullopt;
                }

                return entry.entry_hi.vpn2 | page_offset;
            }
        }

        if(write) 
        {
            bad_vaddr_exception(n64,addr,beyond_all_repair::TLBS);
        }

        else 
        {
            bad_vaddr_exception(n64,addr,beyond_all_repair::TLBL);
        }

        // No match found
        return std::nullopt;
    }

    // Direct mapped just pass the addr straight through
    else
    {
        // NOTE: this only works because both direct mapped sections 
        // are the same size...
        return addr & 0x1FFF'FFFF;
    }
}

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