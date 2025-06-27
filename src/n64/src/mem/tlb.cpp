#include <n64/n64.h>

namespace nintendo64
{

void bad_vaddr_exception(N64& n64, u64 address, u32 code);

enum class tlb_access
{
    write,
    read,
};

void set_tlb_exception_regs(N64& n64, u32 full_vpn, u32 region)
{
    auto& context = n64.cpu.cop0.context;
    auto& xcontext = n64.cpu.cop0.xcontext;
    auto& entry_hi = n64.cpu.cop0.entry_hi;

    context.bad_vpn2 = full_vpn & 0x0007'ffff;
    xcontext.bad_vpn2 = full_vpn;
    xcontext.region = region;
    entry_hi.vpn2 = full_vpn;

}

std::optional<u64> translate_vaddr(N64& n64, u64 addr, tlb_access access) {
    const u16 tlb_set = 0b11'11'00'00'11111111;
    const u32 idx = (addr & 0xf000'0000) >> 28;

    if(!is_set(tlb_set,idx))
    {
        // Direct mapped just pass the addr straight through
        // NOTE: this only works because both direct mapped sections 
        // are the same size...
        return addr & 0x1FFF'FFFF;
    }

    // TLB mapped
    auto& cop0 = n64.cpu.cop0;
    auto& tlb = n64.mem.tlb;

    // For 32 bit mode, 64 bit is just full at 27 bits
    const u64 vpn2_mode_mask = 0x0007'ffff;

    const u32 region = (addr >> 62) & 0b11;
    const u32 vpn2 = (addr >> 13) & vpn2_mode_mask;
    const u32 full_vpn = (addr >> 13) & 0x07ff'ffff;

    // assume a 32 bit address, change this in 64 bit mode?
    addr &= 0xffff'ffff;

    // Scan for a match in the tlb
    for(u32 e = 0; e < TLB_SIZE; e++) 
    {
        auto& entry = tlb.entry[e];

        const bool odd_page = is_set(addr,entry.odd_page_bit);
        auto& entry_lo = odd_page? entry.entry_lo_one : entry.entry_lo_zero;

        const u64 vpn2_mask = ~entry.page_mask;

        const bool match_asid = (cop0.entry_hi.asid == entry.entry_hi.asid) || entry_lo.g;
        const bool match_vpn = (vpn2 & vpn2_mask) == (entry.entry_hi.vpn2 & vpn2_mask);

        if(match_vpn && match_asid)
        {   
            // If the entry is not valid we have a problem!
            if(!entry_lo.v) 
            {
                spdlog::debug("Hit invalid TLB entry for addr {:x} vpn2 {:x}, asid {:x}, g {}, odd page {}({})",addr,vpn2,cop0.entry_hi.asid,entry_lo.g,odd_page,entry.odd_page_bit);
                break;
            }

            // Attempt to write to a read only entry!
            if(!entry_lo.d && access == tlb_access::write)
            {
                set_tlb_exception_regs(n64,full_vpn,region);
                bad_vaddr_exception(n64,addr,beyond_all_repair::TLBM);
                return std::nullopt;
            }

            const u32 page_mask = entry.page_mask << 12;
            const u32 page_offset = addr & (page_mask | 0xfff);
            const u32 translated_addr = ((entry_lo.pfn << 12) & ~page_mask) | page_offset;
            spdlog::trace("Translated addr to {:x} from {:x} ({:x})",translated_addr,addr,page_mask);
            return translated_addr;
        }
    }

    // TLB miss

    switch(access)
    {
        case tlb_access::read:
        {
            set_tlb_exception_regs(n64,full_vpn,region);
            bad_vaddr_exception(n64,addr,beyond_all_repair::TLBL);
            break;
        }

        case tlb_access::write:
        {
            set_tlb_exception_regs(n64,full_vpn,region);
            bad_vaddr_exception(n64,addr,beyond_all_repair::TLBS);
            break;
        }
    }

    // No match found
    return std::nullopt;
}

void write_tlb(N64& n64, u32 idx) 
{
    auto& cop0 = n64.cpu.cop0;
    auto& tlb = n64.mem.tlb;
    
    u32 page_mask_written = cop0.page_mask;
    u32 effective_mask = 0;

    // page mask is set to 0b11 for each pair if the high bit is set
    for(u32 i = 0; i < 12; i += 2)
    {
        const u32 mask = (0b10 << i);
        if((page_mask_written & mask) == mask)
        {
            effective_mask |= (0b11 << i);
        }
    }

    tlb.entry[idx].page_mask = effective_mask;

    const u32 last_set = destoer::fls(tlb.entry[idx].page_mask);
    tlb.entry[idx].odd_page_bit = last_set != FLS_EMPTY? (last_set + 13) : 12;

    tlb.entry[idx].entry_hi = cop0.entry_hi;

    cop0.entry_lo_zero.pfn &= 0xfffff;
    cop0.entry_lo_one.pfn &= 0xfffff;

    tlb.entry[idx].entry_lo_zero = cop0.entry_lo_zero;
    tlb.entry[idx].entry_lo_one = cop0.entry_lo_one;

    const bool g = cop0.entry_lo_zero.g && cop0.entry_lo_one.g;

    tlb.entry[idx].entry_lo_zero.g = g;
    tlb.entry[idx].entry_lo_one.g = g;
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

        const u32 entry_vpn2 = entry.entry_hi.vpn2 & ~entry.page_mask;
        const u32 vpn2 = cop0.entry_hi.vpn2 & ~entry.page_mask;

        // scan for a matching page number with, either a matching asid or ignore
        if((vpn2 == entry_vpn2) && 
            (entry.entry_lo_zero.g || (entry.entry_hi.asid == cop0.entry_hi.asid)) &&
            (entry.entry_hi.region == cop0.entry_hi.region))
        {
            cop0.index.idx = e;
            cop0.index.p = false;
            return;
        }
    }

    cop0.index.idx = 0;
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

    cop0.entry_hi.vpn2 &= ~entry.page_mask;

    cop0.entry_lo_zero = entry.entry_lo_zero;
    cop0.entry_lo_one = entry.entry_lo_one;
}

}