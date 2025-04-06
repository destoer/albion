
namespace nintendo64
{

// NOTE: all intr handling goes here

// see page 151 psuedo code of manual
void standard_exception(N64& n64, u32 code)
{
    // TODO: i think we need to run tests for this LOL
    auto& cop0 = n64.cpu.cop0;
    auto& status = cop0.status;
    auto& cause = cop0.cause;

    if(!status.exl)
    {
        status.exl = true;
        cause.exception_code = code;


        // This assumes we have finished executing an instruction...
        if(!in_delay_slot(n64.cpu))
        {
            cop0.epc = n64.cpu.pc;
            cause.branch_delay = false;
        }

        else
        {
            cop0.epc = n64.cpu.pc - beyond_all_repair::MIPS_INSTR_SIZE;
            cause.branch_delay = true;
        }

        if(is_set(status.ds,6))
        {
            printf("Warning bev set in interrupt\n");
        }

        // bev goes to uncached
        const u64 base = is_set(status.ds,6)? 0xFFFF'FFFF'BFC0'0200 : 0xFFFF'FFFF'8000'0000;

        u32 vector = 0;

        switch(code)
        {
            case beyond_all_repair::TLBL:
            case beyond_all_repair::TLBS:
            case beyond_all_repair::TLBM:
            {
                // 0x80 in 64 bit
                vector = 0x00;
                break;
            }

            default: vector = 0x180; break;
        }

        const u64 target = base + vector; 

        write_pc(n64,target);
        skip_instr(n64.cpu);   
    }

    // double fault (this should not be technically possible)
    else
    {
        assert(false);
    }
}

void coprocesor_unusable(N64& n64, u32 number)
{
    // set coprocessor number, then its just a standard exception
    // with the cop exception code?
    auto& cause = n64.cpu.cop0.cause;
    cause.coprocessor_error = number;
    standard_exception(n64,beyond_all_repair::COP_UNUSABLE);
}

void bad_vaddr_exception(N64& n64, u64 address, u32 code) 
{
    n64.cpu.cop0.bad_vaddr = address;
    standard_exception(n64,code);
}

void check_interrupts(N64 &n64)
{
    auto& cop0 = n64.cpu.cop0;
    auto& status = cop0.status;
    auto& cause = cop0.cause;


    //printf("boop : %x : %d : %d : %d\n",cause.pending,status.ie,status.erl,status.exl);

    // enable off or execption being serviced
    // no interrupts on
    if(!status.ie || status.erl || status.exl)
    {
        return;
    }

    // mask enabled with pending interrupt
    const b32 pending = (status.im & cause.pending) != 0;

    if(pending)
    {
        //print("FIRED: {:x} : {:x}\n",status.im,cause.pending);
        standard_exception(n64,beyond_all_repair::INTERRUPT); 
    }
}


b32 cop0_usable(N64& n64)
{
    auto& status = n64.cpu.cop0.status;

    // coprocesor unusable if disabled and not in kernel mode
    if(!status.cu0 && (status.ksu != KERNEL_MODE))
    {
        coprocesor_unusable(n64,0);
        return false;
    }

    return true;
}


void insert_count_event(N64 &n64)
{
    u64 cycles;

    auto& cop0 = n64.cpu.cop0;

    const u64 count = cop0.count;
    const u64 compare = cop0.compare;

    
    if(count < compare)
    {
        cycles = (compare - count);
    }

    else
    {
        cycles = (u32(0xffff'ffff) - count) + compare; 
    }

    const auto event = n64.scheduler.create_event(cycles * 2,n64_event::count);
    n64.scheduler.insert(event,false); 
}


void set_intr_cop0(N64& n64, u32 bit)
{
    auto& cause = n64.cpu.cop0.cause;

    cause.pending = set_bit(cause.pending,bit);
    check_interrupts(n64);
}

void deset_intr_cop0(N64& n64, u32 bit)
{
    auto& cause = n64.cpu.cop0.cause;

    cause.pending = deset_bit(cause.pending,bit);
}

void count_intr(N64 &n64)
{
    set_intr_cop0(n64,COUNT_BIT);
}

void count_event(N64& n64, u32 cycles)
{
    auto& cop0 = n64.cpu.cop0;
    cop0.count += cycles >> 1;

    // Not due to an event cancelation it should have tripped
    // Due to deffered checking it may have overrun a couple of cycles
    if(!n64.count_cancel)
    {
        spdlog::debug("Count {:x} == {:x}",cop0.count,cop0.compare);
        count_intr(n64);  
        insert_count_event(n64); 
    }

    n64.count_cancel = false;
}

void mi_intr(N64& n64)
{
    set_intr_cop0(n64,MI_BIT);
}

void write_entry_lo(EntryLo& entry_lo, u32 v)
{
    entry_lo.pfn = (v >> 6) & 0x000f'ffff;
    entry_lo.c = (v >> 3) & 0b11;
    entry_lo.d = is_set(v,2);
    entry_lo.v = is_set(v,1);
    entry_lo.g = is_set(v,0);
}

void log_entry_lo(const std::string& TAG,EntryLo& entry_lo)
{
    spdlog::trace("{} pfn 0x{:x}, c {}, d {}, v {}, g {}",TAG,entry_lo.pfn,entry_lo.c,entry_lo.d,entry_lo.v,entry_lo.g);
}

u32 read_entry_lo(EntryLo& entry_lo)
{
    return (entry_lo.g << 0) | (entry_lo.v << 1) | (entry_lo.d << 2) | 
        (entry_lo.c << 3) | (entry_lo.pfn << 6); 
}


void write_cop0(N64 &n64, u64 v, u32 reg)
{
    auto &cpu = n64.cpu;
    auto &cop0 = cpu.cop0;

    switch(reg)
    {
        // cycle counter (incremented every other cycle)
        case beyond_all_repair::COUNT:
        {
            n64.count_cancel = true;
            n64.scheduler.remove(n64_event::count,false);
            cop0.count = v;

            spdlog::trace("COP0 count {:x}", cop0.count);
            insert_count_event(n64);
            n64.count_cancel = false;
            break;
        }

        // when this is == count trigger in interrupt
        case beyond_all_repair::COMPARE:
        {
            n64.count_cancel = true;
            n64.scheduler.remove(n64_event::count);
            
            cop0.compare = v;
            insert_count_event(n64);

            // ip7 in cause is reset when this is written
            deset_intr_cop0(n64,COUNT_BIT);
            spdlog::trace("COP0 compare {:x}",cop0.compare);
            break;
        }
        
        case beyond_all_repair::TAGLO:
        {
            cop0.tagLo = v;
            spdlog::trace("COP0 tagLo",cop0.tagLo);
            break;
        }

        case beyond_all_repair::TAGHI: // write nothing
        {
            break;
        }

        // various cpu settings
        case beyond_all_repair::STATUS:
        {
            auto& status = cop0.status;

            status.ie = is_set(v,0);
            status.exl = is_set(v,1);
            status.erl = is_set(v,2);
            status.ksu = (v >> 3) & 0b11;

            status.ux = is_set(v,5);
            status.sx = is_set(v,6);
            status.kx = is_set(v,7);

            status.im = (v >> 8) & 0xff;

            status.ds = (v >> 16) & 0x1ff;

            status.re = is_set(v,25);
            status.fr = is_set(v,26);
            status.rp = is_set(v,27);

            status.cu0 = is_set(v,28);
            status.cu1 = is_set(v,29);
            status.cu2 = is_set(v,30);
            status.cu3 = is_set(v,31);


            spdlog::trace("COP0 status: ");
            spdlog::trace("ie {}, exl {}, erl {}, ksu {}",status.ie,status.exl,status.erl,status.ksu);
            spdlog::trace("ux {}, sx {}, kx {}",status.ux,status.sx,status.kx);
            spdlog::trace("im {}, ds {}, re {}, fr {}, rp {}",status.im,status.ds,status.re,status.fr,status.rp);
            spdlog::trace("cu0 {}, cu1 {}, cu2 {}, cu3 {}",status.cu0,status.cu1,status.cu2,status.cu3);

            if(status.rp)
            {
                unimplemented("low power mode");
            }

            if(status.re)
            {
                unimplemented("little endian");
            }

            // TODO: we need to cache this inside address translation if it trips
            if((status.ux && status.ksu == 0b10) || (status.sx && status.ksu == 0b01) || (status.kx && status.ksu == 0b00))
            {
                unimplemented("64 bit addressing");
            }

            check_interrupts(n64);
            break;
        }

        // interrupt / exception info
        case beyond_all_repair::CAUSE:
        {
            auto& cause = cop0.cause;

            const auto PENDING_MASK = 0b11;

            // write can only modify lower 2 bits of interrupt pending
            cause.pending = (cause.pending & ~PENDING_MASK)  | ((v >> 8) & PENDING_MASK);
            spdlog::trace("COP0 Cause pennding: 0x{:x}",cause.pending);
            check_interrupts(n64);
            break;
        }

        case beyond_all_repair::ENTRY_HI:
        {
            auto& entry_hi = cop0.entry_hi;
            entry_hi.asid = v & 0xff; 
            entry_hi.vpn2 = (v >> 13) & 0x7'ff'ff'ff;
            entry_hi.region = (v >> 62) & 0b11;
            spdlog::trace("COP0 entry_hi asid 0x{:x}, vpn2 0x{:x}, region 0x{:x}",entry_hi.asid,entry_hi.vpn2,entry_hi.region);
            break;
        }

        case beyond_all_repair::ENTRY_LO_ZERO:
        {
            write_entry_lo(cop0.entry_lo_zero,v);
            log_entry_lo("COP0 EntryLoZero",cop0.entry_lo_zero);
            break;
        }

        case beyond_all_repair::ENTRY_LO_ONE:
        {
            write_entry_lo(cop0.entry_lo_one,v);
            log_entry_lo("COP0 EntryLoOne",cop0.entry_lo_one);
            break;
        }

        case beyond_all_repair::CONFIG:
        {
            cop0.config.transfer_mode = (v >> 24 & 0b111);
            cop0.config.endianness = is_set(v, 14);
            cop0.config.cu = is_set(v, 2);
            cop0.config.k0 = v & 0b11;
            spdlog::trace("COP0 Config transfer mode {}, endianess {}, cu {}, k0 {}",cop0.config.transfer_mode,cop0.config.endianness,cop0.config.cu,cop0.config.k0);
            break;
        }

        case beyond_all_repair::XCONFIG:
        {
            cop0.xconfig.pte = v >> 32;
            spdlog::trace("COP0 xconfig pte 0x{:x}",cop0.xconfig.pte);
            break;
        }

        case beyond_all_repair::WIRED:
        {
            cop0.wired = (v >> 5) & 0b11111;
            cop0.random = 31;
            spdlog::trace("COP0 Wired wire {}, random {}",cop0.wired,cop0.random);
            break;
        }

        case beyond_all_repair::INDEX:
        {
            auto& index = cop0.index;
            index.p = is_set(v,31);
            index.idx = v & 0b111'111;
            spdlog::trace("COP0 Index p {}, idx {}",index.p,index.idx);
            break;
        }

        case beyond_all_repair::PAGE_MASK:
        {
            cop0.page_mask = (v >> 13) & 0xfff;
            spdlog::trace("COP0 Page mask 0x{:x}",cop0.page_mask);
            break;
        }

        case beyond_all_repair::EPC:
        {
            cop0.epc = v;
            spdlog::trace("COP0 EPC 0x{:x}",cop0.epc);
            break;
        }

        case beyond_all_repair::ERROR_EPC:
        {
            cop0.error_epc = v;
            spdlog::trace("COP0 Error EPC 0x{:x}",cop0.error_epc);
            break;
        }

        case beyond_all_repair::BAD_VADDR:
        {
            cop0.bad_vaddr = v;
            spdlog::trace("COP0 Bad vaddr 0x{:x}",cop0.bad_vaddr);
            break;
        }

        case beyond_all_repair::LLADDR:
        {
            cop0.load_linked = v;
            spdlog::trace("COP0 LLADDR 0x{:x}",cop0.load_linked);
            break;   
        }

        case beyond_all_repair::WATCH_LO:
        {
            cop0.watchLo.paddr0 = v >> 3;
            cop0.watchLo.read = is_set(v,1);
            cop0.watchLo.write = is_set(v,0);
            spdlog::trace("COP0 WATCH_LO paddr0 0x{:x}, read {}, write {}",cop0.watchLo.paddr0,cop0.watchLo.read,cop0.watchLo.write);
            break;
        }

        case beyond_all_repair::WATCH_HI:
        {
            cop0.watchHi.paddr1 = v & 0b111;
            spdlog::trace("COP0 WATCH_HI paddr1 0x{:x}",cop0.watchHi.paddr1);
            break;
        }

        // read only
        case beyond_all_repair::RANDOM:
        case beyond_all_repair::PRID:
        case beyond_all_repair::PARITY_ERROR:
        case beyond_all_repair::CACHE_ERROR:
        case 7: case 21: case 22: case 23: case 24: case 25: case 31:
            break;

        default:
        {
            printf("unimplemented cop0 write: %s(%d)\n",beyond_all_repair::COP0_NAMES[reg],reg);
            exit(1);
        }
    }
}

u64 read_cop0(N64& n64, u32 reg)
{
    using namespace beyond_all_repair;

    auto& cpu = n64.cpu;
    auto& cop0 = cpu.cop0;

    switch(reg)
    {
        case RANDOM:
        {
            return cop0.random;
        }

        case LLADDR:
        {
            return cop0.load_linked;
        }

        case WATCH_LO:
        {
            return cop0.watchLo.paddr0 << 3 | (cop0.watchLo.read & 1) << 1 | (cop0.watchLo.write & 1);
        }

        case WATCH_HI:
        {
            return cop0.watchHi.paddr1 & 0b111;
        }

        case XCONFIG:
        {
            return (u64) cop0.xconfig.pte << 33 | (cop0.xconfig.r & 0b11) << 31 | cop0.xconfig.bad_vpn << 3;
        }

        case PARITY_ERROR:
        {
            return cop0.parity;
        }

        case CACHE_ERROR:
        {
            return 0;
        }

        case STATUS:
        {
            auto& status = cop0.status;

            return status.ie | (status.exl << 1) | (status.erl << 2) |
                (status.ksu << 3) | (status.ux << 5) | (status.sx << 6) |
                (status.kx << 7) | (status.im << 8) | (status.ds << 16) |
                (status.re << 25) | (status.fr << 26) | (status.rp << 27) |
                (status.cu0 << 28) | (status.cu1 << 29) | (status.cu2 << 30) | 
                (status.cu3 << 31);
        }

        case ENTRY_HI:
        {
            auto& entry_hi = cop0.entry_hi;
            return (entry_hi.asid << 0) | (entry_hi.vpn2 << 13);
        }

        case ENTRY_LO_ZERO:
        {
            return read_entry_lo(cop0.entry_lo_zero);
        }

        case ENTRY_LO_ONE:
        {
            return read_entry_lo(cop0.entry_lo_one);
        }

        case PAGE_MASK:
        {
            return (cop0.page_mask << 13);
        }

        case TAGLO:
        {
            return cop0.tagLo;
        }

        case TAGHI:
        {
            return 0;
        }

        case COUNT:
        {
            n64.count_cancel = true;
            // read out count tick off all the cycles
            n64.scheduler.remove(n64_event::count);
            insert_count_event(n64);

            return cop0.count;
        }

        case EPC:
        {
            return cop0.epc;
        }

        case CAUSE:
        {
            auto& cause = cop0.cause;
            return (cause.exception_code << 2) | (cause.pending << 8) | (cause.coprocessor_error << 28) | (cause.branch_delay << 31); 
        }

        case CONFIG:
        {
            auto& config = cop0.config;
            return config.freq << 28 | config.transfer_mode << 24 | 0b1101 << 15 | config.endianness << 14 | 0b11001000110 << 4 | config.cu << 2 | config.k0;
        }

        case PRID:
        {
            return cop0.prid;
        }

        case INDEX:
        {
            auto& index = cop0.index;
            return (index.idx << 0) | (index.p << 31);
        }

        case ERROR_EPC:
        {
            return cop0.error_epc;
        }

        case BAD_VADDR:
        {
            return cop0.bad_vaddr;
        }

        case CONTEXT:
        {
            auto& context = cop0.context;
            return (context.bad_vpn2 << 4) | (context.pte_base << 23);
        }

        case COMPARE:
        {
            return cop0.compare;
        }

        case WIRED:
        {
            return cop0.wired;
        }

        default:
        {
            spdlog::debug("cop0 unknown: {}\n",reg);
            return 0;
        }        
    }
}

void update_random(Cop0& cop0) 
{
    if(cop0.random == 0)
    {
        cop0.random = 31;
    }

    else
    {
        cop0.random -= 1;
    }
}

}