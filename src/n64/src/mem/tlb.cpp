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

        // assume a 32 bit address, change this in 64 bit mode?
        addr &= 0xffff'ffff;

        // Scan for a match in the tlb
        for(u32 e = 0; e < TLB_SIZE; e++) 
        {
            auto& entry = tlb.entry[e];

            // For 32 bit mode, 64 bit is just full at 27 bits
            const u64 vpn2_mode_mask = 0x7ffff;
            const u64 vpn2_mask = vpn2_mode_mask & ~entry.page_mask;

            auto& entry_lo = is_set(addr,13)? entry.entry_lo_one : entry.entry_lo_zero;
            const auto vpn2 = ((addr >> 12) & vpn2_mask);
            
            // If the entry is not valid throw an exception
            if(!entry_lo.v) 
            {
                break;
            }

            if((entry.entry_hi.vpn2 & vpn2_mask) == vpn2 && (cop0.entry_hi.asid == entry.entry_hi.asid || entry_lo.g)) 
            {   
                // Attempt to write to a read only entry!
                if(!entry_lo.d && write)
                {
                    bad_vaddr_exception(n64,addr,beyond_all_repair::TLBM);
                    return std::nullopt;
                }

                const auto page_offset = addr & ((entry.page_mask << 12) | 0xfff);
                return (entry_lo.pfn << 12) | page_offset;
            }
        }

        // TLB miss

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
    tlb.entry[idx].entry_hi.vpn2 = tlb.entry[idx].entry_hi.vpn2;
    
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
            cop0.index.p = false;
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