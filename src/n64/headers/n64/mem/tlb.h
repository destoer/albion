
#include "n64/cpu.h"
namespace nintendo64
{

static constexpr u32 TLB_SIZE = 64;

struct TLBEntry
{
    u32 page_mask = 0;
    EntryHi entry_hi;
    EntryLo entry_lo_one;
    EntryLo entry_lo_zero;
};

struct TLB
{
    TLBEntry entry[TLB_SIZE];
};

}