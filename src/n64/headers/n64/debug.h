#pragma once
#include <albion/debug.h>
#include <albion/lib.h>
#include <n64/forward_def.h>


namespace nintendo64
{
class N64Debug final : public Debug 
{
public:
    u64 last_call = 0;

    N64Debug(N64 &n64);


    // standard commands
    void regs(const std::vector<Token> &args);
    void step(const std::vector<Token> &args);
    void disass(const std::vector<Token> &args);    

    void disass_func(const std::vector<Token> &args);    




    // overrides
    void change_breakpoint_enable(bool enable) override;
    uint8_t read_mem(uint64_t addr) override;
    void write_mem(u64 addr, u8 v) override;
    std::string disass_instr(uint64_t addr) override;
    uint64_t get_instr_size(uint64_t addr) override;
    void execute_command(const std::vector<Token> &args) override;
    b32 read_var(const std::string &name, u64* out) override;
    void step_internal() override;
    void on_break() override;

private:

    using COMMAND_FUNC =  void (N64Debug::*)(const std::vector<Token>&);
    std::unordered_map<std::string,COMMAND_FUNC> func_table =
    {
        {"break",&N64Debug::breakpoint},
        {"run", &N64Debug::run},
        {"regs",&N64Debug::regs},
        {"step",&N64Debug::step},
        {"disass",&N64Debug::disass_internal},
        {"trace",&N64Debug::print_trace},
        {"mem",&N64Debug::print_mem},
        {"break_clear",&N64Debug::clear_breakpoint},
        {"break_enable",&N64Debug::enable_breakpoint},
        {"break_disable",&N64Debug::disable_breakpoint},
        {"break_list",&N64Debug::list_breakpoint},
        {"watch",&N64Debug::watch},
        {"watch_enable",&N64Debug::enable_watch},
        {"watch_disable",&N64Debug::disable_watch},
        {"watch_list",&N64Debug::list_watchpoint},
        {"log_trace",&N64Debug::log_trace},
        {"log_debug",&N64Debug::log_debug},
        {"log_info",&N64Debug::log_info},
        {"disass_func",&N64Debug::disass_func}
    };

    N64 &n64;
};

}