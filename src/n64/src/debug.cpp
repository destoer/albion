
#include <n64/n64.h>

namespace nintendo64
{

N64Debug::N64Debug(N64 &n) : n64(n)
{

}

void N64Debug::execute_command(const std::vector<Token> &args)
{
    if(invalid_command(args))
    {
        print_console("invalid command\n");
        return;
    }

    const auto command = std::get<std::string>(args[0]);
    if(!func_table.count(command))
    {
        print_console("unknown command: '{}'\n",command);
        return;
    }

    const auto func = func_table[command];

    std::invoke(func,this,args);
}


// these are better off being completly overriden
void N64Debug::regs(const std::vector<Token> &args)
{
    UNUSED(args);
    
    printf("pc = %016lx\n",n64.cpu.pc);

    for(u32 i = 0; i < 32; i++)
    {
        printf("%s = %016lx\n",beyond_all_repair::REG_NAMES[i],n64.cpu.regs[i]);
    }
}

void N64Debug::step_internal()
{
    const auto old = breakpoints_enabled;
    breakpoints_enabled = false;
    nintendo64::step<false>(n64);
    breakpoints_enabled = old;
    halt();
}

void N64Debug::step(const std::vector<Token> &args)
{
    UNUSED(args);
    const auto pc = read_pc();
    const auto instr = disass_instr(pc);
    print_console("{}\n",instr);
    step_internal();
}

std::string N64Debug::disass_instr(u64 addr)
{
    const u32 opcode = read_mem_raw<u32>(n64,addr);

    const Opcode op = beyond_all_repair::make_opcode(opcode);  

    return fmt::format("{:x}: {}",addr,disass_n64(n64,op,addr+sizeof(u32)));
}


void N64Debug::disass(const std::vector<Token> &args)
{
    UNUSED(args);
    disass_internal(args);
}

u64 N64Debug::get_instr_size(u64 addr)
{
    UNUSED(addr);
    return sizeof(u32);
}


u8 N64Debug::read_mem(u64 addr)
{
    return read_mem_raw<u8>(n64,addr);
}

void N64Debug::write_mem(u64 addr, u8 v)
{
    write_mem_raw<u8>(n64,addr,v);
}

void N64Debug::change_breakpoint_enable(bool enable)
{
    n64.debug_enabled = enable;
}

b32 N64Debug::read_var(const std::string &name, u64* out)
{
    b32 success = true;

    if(name == "pc")
    {
        *out = n64.cpu.pc;
    }

    else
    {
        // TODO: we could make this faster
        for(u32 i = 0; i < beyond_all_repair::REG_NAMES_SIZE; i++)
        {
            if(beyond_all_repair::REG_NAMES[i] == name)
            {
                *out = n64.cpu.regs[i];
                return true;
            }
        }


        *out = 0;
        success = false;
    }

    return success;
}

void print_func_disass(N64& n64, u64 target)
{
    beyond_all_repair::Config config;
    config.print_addr = true;
    config.print_opcodes = true;

    auto& func = beyond_all_repair::add_func_no_context(n64.program,target);
    beyond_all_repair::disassemble_func(n64.program,func);
    beyond_all_repair::print_console_func_mips(n64.program,func,config,n64.cpu.pc);

    // Don't want this hanging around
    beyond_all_repair::clear_references(n64.program);
}

void N64Debug::disass_func(const std::vector<Token> &args)
{
    u64 target = last_call;

    if(args.size() >= 2 && read_type(args[1]) == token_type::u64_t)
    {
        target = read_token_u64(args[1]);
    }


    print_func_disass(n64,target);
}

void N64Debug::on_break()
{
    // print_func_disass(n64,n64.cpu.pc_fetch);
}

}