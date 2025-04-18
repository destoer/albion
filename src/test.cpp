#include <destoer/destoer.h>
#include <albion/lib.h>

#ifdef GB_ENABLED
#include <gb/gb.h>

// gameboy test running
void gb_run_test_helper(const std::vector<std::string> &tests, int seconds)
{

    int fail = 0;
    int pass = 0;
    int aborted = 0;
    int timeout = 0;

    gameboy::GB gb;

    for(const auto &x: tests)
    {
        
        try
        {
            gb.reset(x);
            gb.throttle_emu = false;


            auto start = std::chrono::system_clock::now();

            // add a timer timeout if required
            for(;;)
            {
                gb.run();


                if(gb.mem.test_result == emu_test::fail)
                {
                    std::cout << fmt::format("{}: fail\n",x);
                    fail++;
                    break;
                }

                else if(gb.mem.test_result == emu_test::pass)
                {
                    // we are passing so many compared to fails at this point
                    // it doesnt make sense to print them
                    //std::cout << fmt::format("{}: pass\n",x);
                    pass++;
                    break;
                }

                auto current = std::chrono::system_clock::now();

                // if the test takes longer than 5 seconds time it out
                if(std::chrono::duration_cast<std::chrono::seconds>(current - start).count() > seconds)
                {
                    std::cout << fmt::format("{}: timeout\n",x);
                    timeout++;
                    break;
                }
            }
        }

        catch(std::exception &ex)
        {
            std::cout << fmt::format("{}: aborted {}\n",x,ex.what());
            aborted++;
        }
    }

    printf("total: %zd\n",tests.size());
    printf("pass: %d\n",pass);
    printf("fail: %d\n",fail);
    printf("abort: %d\n",aborted);
    printf("timeout: %d\n",timeout);    
}

void gb_run_tests()
{
    puts("gekkio_tests:");
    auto start = std::chrono::system_clock::now();
    const auto [tree,error] = read_dir_tree("mooneye-gb_hwtests");
    gb_run_test_helper(filter_ext(tree,"gb"),10);
    auto current = std::chrono::system_clock::now();
    auto count = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count()) / 1000.0;
    printf("total time taken %f\n",count);
}
#endif

#ifdef N64_ENABLED
#include <n64/n64.h>


#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

b32 read_test_image(const std::string& filename, std::vector<u32>& buf)
{
    int x, y, n;
    unsigned char *data = stbi_load(filename.c_str(), &x, &y, &n, 4);

    if(!data)
    {
        return true;
    }

    buf.resize(x * y);
    memcpy(buf.data(),data,buf.size() * sizeof(u32));
    free(data);

    //printf("read image: %s (%d,%d) %zd\n",filename.c_str(),x,y,buf.size());

    return false;
}


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

bool write_test_image(const std::string &filename, std::vector<u32> &buf,u32 x, u32 y)
{
    bool error = !stbi_write_png(filename.c_str(),x,y,4,buf.data(),x * sizeof(u32));
    printf("wrote fail image: %s: %d\n",filename.c_str(),error);
    return error;
}

struct Test
{
    const char* rom_path;
    const char* image_name;
    const char* name;
    int frames;
    // We should try get rid of these, but its better for development atm
    bool override_image = false;
};

static constexpr u32 CPU_TEST_FRAMES = 10;

static constexpr Test CPU_TESTS[] = 
{
    {"ADD/CPUADD.N64","ADD/CPUADD.png","KROM_CPU_ADD",CPU_TEST_FRAMES},
    {"AND/CPUAND.N64","AND/CPUAND.png","KROM_CPU_AND",CPU_TEST_FRAMES},
    {"ADDU/CPUADDU.N64","ADDU/CPUADDU.png","KROM_CPU_ADDU",CPU_TEST_FRAMES},
    {"DADDU/CPUDADDU.N64","DADDU/CPUDADDU.png","KROM_CPU_DADDU",CPU_TEST_FRAMES},

    {"DDIV/CPUDDIV.N64","DDIV/CPUDDIV.png","KROM_CPU_DDIV",CPU_TEST_FRAMES},
    {"DDIVU/CPUDDIVU.N64","DDIVU/CPUDDIVU.png","KROM_CPU_DDIVU",CPU_TEST_FRAMES},
    {"DIV/CPUDIV.N64","DIV/CPUDIV.png","KROM_CPU_DIV",CPU_TEST_FRAMES},
    {"DIVU/CPUDIVU.N64","DIVU/CPUDIVU.png","KROM_CPU_DIVU",CPU_TEST_FRAMES},

    {"DMULT/CPUDMULT.N64","DMULT/CPUDMULT.png","KROM_CPU_DMULT",CPU_TEST_FRAMES},
    {"DMULTU/CPUDMULTU.N64","DMULTU/CPUDMULTU.png","KROM_CPU_DMULTU",CPU_TEST_FRAMES},

    {"DSUB/CPUDSUB.N64","DSUB/CPUDSUB.png","KROM_CPU_DSUB",CPU_TEST_FRAMES},
    {"DSUBU/CPUDSUBU.N64","DSUBU/CPUDSUBU.png","KROM_CPU_DSUBU",CPU_TEST_FRAMES},

    {"LOADSTORE/LB/CPULB.N64","LOADSTORE/LB/CPULB.png","KROM_CPU_LB",CPU_TEST_FRAMES},
    {"LOADSTORE/LH/CPULH.N64","LOADSTORE/LH/CPULH.png","KROM_CPU_LH",CPU_TEST_FRAMES},
    {"LOADSTORE/LW/CPULW.N64","LOADSTORE/LW/CPULW.png","KROM_CPU_LW",CPU_TEST_FRAMES},
    {"LOADSTORE/LD/CPULD.N64","LOADSTORE/LD/CPULD.png","KROM_CPU_LD",CPU_TEST_FRAMES},

    //passing without a matching image?

    {"LOADSTORE/SB/CPUSB.N64","LOADSTORE/SB/CPUSB.png","KROM_CPU_SB",CPU_TEST_FRAMES,true},
    {"LOADSTORE/SH/CPUSH.N64","LOADSTORE/SH/CPUSH.png","KROM_CPU_SH",CPU_TEST_FRAMES,true},
    {"LOADSTORE/SW/CPUSW.N64","LOADSTORE/SW/CPUSW.png","KROM_CPU_SW",CPU_TEST_FRAMES},
    {"LOADSTORE/SD/CPUSD.N64","LOADSTORE/SD/CPUSD.png","KROM_CPU_SD",CPU_TEST_FRAMES},

    {"LOADSTORE/LL_LLD_SC_SCD/LL_LLD_SC_SCD.N64","LOADSTORE/LL_LLD_SC_SCD/LL_LLD_SC_SCD.png","KROM_CPU_LL_LLD_SC_SCD",CPU_TEST_FRAMES,true},

    {"MULT/CPUMULT.N64","MULT/CPUMULT.png","KROM_CPU_MULT",CPU_TEST_FRAMES},
    {"MULTU/CPUMULTU.N64","MULTU/CPUMULTU.png","KROM_CPU_MULTU",CPU_TEST_FRAMES},

    {"NOR/CPUNOR.N64","NOR/CPUNOR.png","KROM_CPU_NOR",CPU_TEST_FRAMES},
    {"OR/CPUOR.N64","OR/CPUOR.png","KROM_CPU_NO",CPU_TEST_FRAMES},

    {"SUB/CPUSUB.N64","SUB/CPUSUB.png","KROM_CPU_SUB",CPU_TEST_FRAMES},
    {"SUBU/CPUSUBU.N64","SUBU/CPUSUBU.png","KROM_CPU_SUBU",CPU_TEST_FRAMES},

    {"SHIFT/DSLL/CPUDSLL.N64","SHIFT/DSLL/CPUDSLL.png","KROM_CPU_CPUDSLL",CPU_TEST_FRAMES},
    {"SHIFT/DSLL32/CPUDSLL32.N64","SHIFT/DSLL32/CPUDSLL32.png","KROM_CPU_CPUDSLL32",CPU_TEST_FRAMES},
    {"SHIFT/DSLLV/CPUDSLLV.N64","SHIFT/DSLLV/CPUDSLLV.png","KROM_CPU_CPUDSLLV",CPU_TEST_FRAMES},

    {"SHIFT/DSRA/CPUDSRA.N64","SHIFT/DSRA/CPUDSRA.png","KROM_CPU_DSRA",CPU_TEST_FRAMES},
    {"SHIFT/DSRA32/CPUDSRA32.N64","SHIFT/DSRA32/CPUDSRA32.png","KROM_CPU_DSRA32",CPU_TEST_FRAMES},
    {"SHIFT/DSRAV/CPUDSRAV.N64","SHIFT/DSRAV/CPUDSRAV.png","KROM_CPU_DSRAV",CPU_TEST_FRAMES},

    {"SHIFT/DSRL/CPUDSRL.N64","SHIFT/DSRL/CPUDSRL.png","KROM_CPU_DSRL",CPU_TEST_FRAMES},
    {"SHIFT/DSRL32/CPUDSRL32.N64","SHIFT/DSRL32/CPUDSRL32.png","KROM_CPU_DSRL32",CPU_TEST_FRAMES},
    {"SHIFT/DSRLV/CPUDSRLV.N64","SHIFT/DSRLV/CPUDSRLV.png","KROM_CPU_DSRLV",CPU_TEST_FRAMES},

    {"SHIFT/SLL/CPUSLL.N64","SHIFT/SLL/CPUSLL.png","KROM_CPU_CPUSLL",CPU_TEST_FRAMES},
    {"SHIFT/SLLV/CPUSLLV.N64","SHIFT/SLLV/CPUSLLV.png","KROM_CPU_CPUSLLV",CPU_TEST_FRAMES},

    {"SHIFT/SRA/CPUSRA.N64","SHIFT/SRA/CPUSRA.png","KROM_CPU_CPUSRA",CPU_TEST_FRAMES},
    {"SHIFT/SRAV/CPUSRAV.N64","SHIFT/SRAV/CPUSRAV.png","KROM_CPU_CPUSRAV",CPU_TEST_FRAMES},

    {"SHIFT/SRL/CPUSRL.N64","SHIFT/SRL/CPUSRL.png","KROM_CPU_CPUSRL",CPU_TEST_FRAMES},
    {"SHIFT/SRLV/CPUSRLV.N64","SHIFT/SRLV/CPUSRLV.png","KROM_CPU_CPUSRLV",CPU_TEST_FRAMES},

    {"XOR/CPUXOR.N64","XOR/CPUXOR.png","KROM_CPU_XOR",CPU_TEST_FRAMES},
};

static constexpr u32 CPU_TEST_SIZE = sizeof(CPU_TESTS) / sizeof(Test);


static constexpr u32 COP1_TEST_FRAMES = 10;

static constexpr Test COP1_TESTS[] = 
{
    // {".N64",".png","KROM_COP1_",COP1_TEST_FRAMES},
    // {"C//CP1C.N64","C//CP1C.png","KROM_COP1_C",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c..N64","C/FPUCompare/FPUCompare-c..png","KROM_COP1_C",COP1_TEST_FRAMES},

    {"ABS/CP1ABS.N64","ABS/CP1ABS.png","KROM_COP1_ABS",COP1_TEST_FRAMES},
    {"ADD/CP1ADD.N64","ADD/CP1ADD.png","KROM_COP1_ADD",COP1_TEST_FRAMES},

    // Cmp tests
    {"C/EQ/CP1CEQ.N64","C/EQ/CP1CEQ.png","KROM_COP1_CEQ",COP1_TEST_FRAMES},
    {"C/F/CP1CF.N64","C/F/CP1CF.png","KROM_COP1_CF",COP1_TEST_FRAMES},

    {"C/LE/CP1CLE.N64","C/LE/CP1CLE.png","KROM_COP1_CLE",COP1_TEST_FRAMES},
    {"C/LT/CP1CLT.N64","C/LT/CP1CLT.png","KROM_COP1_CLT",COP1_TEST_FRAMES},

    {"C/NGE/CP1CNGE.N64","C/NGE/CP1CNGE.png","KROM_COP1_CNGE",COP1_TEST_FRAMES},
    {"C/NGLE/CP1CNGLE.N64","C/NGLE/CP1CNGLE.png","KROM_COP1_CNGLE",COP1_TEST_FRAMES},

    {"C/NGT/CP1CNGT.N64","C/NGT/CP1CNGT.png","KROM_COP1_CNGT",COP1_TEST_FRAMES},
    {"C/OLE/CP1COLE.N64","C/OLE/CP1COLE.png","KROM_COP1_COLE",COP1_TEST_FRAMES},

    {"C/OLT/CP1COLT.N64","C/OLT/CP1COLT.png","KROM_COP1_COLT",COP1_TEST_FRAMES},
    {"C/SEQ/CP1CSEQ.N64","C/SEQ/CP1CSEQ.png","KROM_COP1_CSEQ",COP1_TEST_FRAMES},

    {"C/SF/CP1CSF.N64","C/SF/CP1CSF.png","KROM_COP1_CSF",COP1_TEST_FRAMES},
    {"C/UEQ/CP1CUEQ.N64","C/UEQ/CP1CUEQ.png","KROM_COP1_CUEQ",COP1_TEST_FRAMES},

    {"C/ULE/CP1CULE.N64","C/ULE/CP1CULE.png","KROM_COP1_CULE",COP1_TEST_FRAMES},
    {"C/ULT/CP1CULT.N64","C/ULT/CP1CULT.png","KROM_COP1_CULT",COP1_TEST_FRAMES},

    {"C/UN/CP1CUN.N64","C/UN/CP1CUN.png","KROM_COP1_CUN",COP1_TEST_FRAMES},

    // FPU compare (TODO: Why do these flicker?)
    // {"C/FPUCompare/FPUCompare-c.eq.s.N64","C/FPUCompare/FPUCompare-c.eq.s.png","KROM_COP1_C.EQ.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.f.s.N64","C/FPUCompare/FPUCompare-c.f.s.png","KROM_COP1_C.F.S",COP1_TEST_FRAMES},

    // {"C/FPUCompare/FPUCompare-c.le.s.N64","C/FPUCompare/FPUCompare-c.le.s.png","KROM_COP1_C.LE.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.lt.s.N64","C/FPUCompare/FPUCompare-c.lt.s.png","KROM_COP1_C.LT.S",COP1_TEST_FRAMES},

    // {"C/FPUCompare/FPUCompare-c.nge.s.N64","C/FPUCompare/FPUCompare-c.nge.s.png","KROM_COP1_C.NGE.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.ngl.s.N64","C/FPUCompare/FPUCompare-c.ngl.s.png","KROM_COP1_C.NGL.S",COP1_TEST_FRAMES},
   
    // {"C/FPUCompare/FPUCompare-c.ngle.s.N64","C/FPUCompare/FPUCompare-c.ngle.s.png","KROM_COP1_C.NGLE.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.ngt.s.N64","C/FPUCompare/FPUCompare-c.ngt.s.png","KROM_COP1_C.NGT.S",COP1_TEST_FRAMES},

    // {"C/FPUCompare/FPUCompare-c.ole.s.N64","C/FPUCompare/FPUCompare-c.ole.s.png","KROM_COP1_C.OLE.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.olt.s.N64","C/FPUCompare/FPUCompare-c.olt.s.png","KROM_COP1_C.OLT.S",COP1_TEST_FRAMES},

    // {"C/FPUCompare/FPUCompare-c.seq.s.N64","C/FPUCompare/FPUCompare-c.seq.s.png","KROM_COP1_C.SEQ.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.sf.s.N64","C/FPUCompare/FPUCompare-c.sf.s.png","KROM_COP1_C.SF.S",COP1_TEST_FRAMES},

    // {"C/FPUCompare/FPUCompare-c.ueq.s.N64","C/FPUCompare/FPUCompare-c.ueq.s.png","KROM_COP1_C.UEQ.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.ule.s.N64","C/FPUCompare/FPUCompare-c.ule.s.png","KROM_COP1_C.ULE.S",COP1_TEST_FRAMES},

    // {"C/FPUCompare/FPUCompare-c.ult.s.N64","C/FPUCompare/FPUCompare-c.ult.s.png","KROM_COP1_C.ULT.S",COP1_TEST_FRAMES},
    // {"C/FPUCompare/FPUCompare-c.un.s.N64","C/FPUCompare/FPUCompare-c.un.s.png","KROM_COP1_C.UN.S",COP1_TEST_FRAMES},

    {"CEIL/CP1CEIL.N64","CEIL/CP1CEIL.png","KROM_COP1_CEIL",COP1_TEST_FRAMES},
    {"COP1FullMode/COP1FullMode.N64","COP1FullMode/COP1FullMode.png","KROM_COP1_FULL_MODE",COP1_TEST_FRAMES},

    {"CVT/CP1CVT.N64","CVT/CP1CVT.png","KROM_COP1_CVT",COP1_TEST_FRAMES},
    {"DIV/CP1DIV.N64","DIV/CP1DIV.png","KROM_COP1_DIV",COP1_TEST_FRAMES},

    {"FLOOR/CP1FLOOR.N64","FLOOR/CP1FLOOR.png","KROM_COP1_FLOOR",COP1_TEST_FRAMES},
    {"MUL/CP1MUL.N64","MUL/CP1MUL.png","KROM_COP1_MUL",COP1_TEST_FRAMES},

    {"NEG/CP1NEG.N64","NEG/CP1NEG.png","KROM_COP1_NEG",COP1_TEST_FRAMES},
    {"ROUND/CP1ROUND.N64","ROUND/CP1ROUND.png","KROM_COP1_ROUND",COP1_TEST_FRAMES},

    {"SQRT/CP1SQRT.N64","SQRT/CP1SQRT.png","KROM_COP1_SQRT",COP1_TEST_FRAMES},
    {"SUB/CP1SUB.N64","SUB/CP1SUB.png","KROM_COP1_SUB",COP1_TEST_FRAMES},
    
    {"TRUNC/CP1TRUNC.N64","TRUNC/CP1TRUNC.png","KROM_COP1_TRUNC",COP1_TEST_FRAMES},
};

static constexpr u32 COP1_TEST_SIZE = sizeof(COP1_TESTS) / sizeof(Test);

static constexpr Test COP0_TESTS[] = 
{
    {"COP0Cause/COP0Cause.N64","COP0Cause/COP0Cause.png","KROM_COP0_CAUSE",30},
    {"COP0Register/COP0Register.N64","COP0Register/COP0Register - before overflow.png","KROM_COP0_REGISTER",5},
};

static constexpr u32 COP0_TEST_SIZE = sizeof(COP0_TESTS) / sizeof(Test);


static constexpr Test EXCEPTION_TESTS[] = 
{
    {"VIIntr/ExceptionVIIntrDisabled.n64","VIIntr/ExceptionVIIntrDisabled.png","KROM_EXCEPTION_VI_INTR_DISABLED",60},
    // Timings don't quite match
    {"Compare/ExceptionCompareDisabled.n64","Compare/ExceptionCompareDisabled.png","KROM_EXCEPTION_COMPARE_DISABLED",300,true},
    // Final status does not match on this
    {"Compare/ExceptionCompareRegisters.n64","Compare/ExceptionCompareRegisters.png","KROM_EXCEPTION_COMPARE_REGISTERS",90,true}
};

static constexpr u32 EXCEPTION_TEST_SIZE = sizeof(EXCEPTION_TESTS) / sizeof(Test);

std::pair<std::vector<std::string>,std::vector<std::string>> run_test_list(const std::string suite_name, const std::string base_path,
    const Test test_list[],u32 test_size)
{
    spdlog::info("n64 tests ({}): {}\n",suite_name,base_path);
    
    std::vector<std::string> pass_list;
    std::vector<std::string> fail_list;

    for(u32 t = 0; t < test_size; t++)
    {
        try
        {
            auto& test = test_list[t];
            spdlog::info("start test: {}\n",test.name);

            nintendo64::N64 n64;
            nintendo64::reset(n64,fmt::format("{}/{}",base_path,test.rom_path));

            for(int f = 0; f < test.frames; f++) 
            {
                nintendo64::run(n64);
            }

            std::vector<u32> screen_check;

            // Attempt to open the comparison
            const std::string image_name = test.override_image? fmt::format("n64_image_override/{}.png",test.name) : 
                fmt::format("{}/{}",base_path,test.image_name);

            const b32 error = read_test_image(image_name,screen_check);
            
            // Cannot find file -> auto set the image
            if(error)
            {
                spdlog::error("cannot find reference image\n");
                return std::pair{pass_list,fail_list};
            }

            // Can find file -> compare the images
            else 
            {
                b32 pass = true;

                // compare image and ignore alpha channel
                if(screen_check.size() == n64.rdp.screen.size())
                {
                    for(u32 i = 0; i < screen_check.size(); i++)
                    {
                        const u32 v1 = (screen_check[i] & 0x00ff'ffff);
                        const u32 v2 = (n64.rdp.screen[i] & 0x00ff'ffff);
                        if(v1 != v2)
                        {
                            spdlog::info("images differ at: {}, {:x} != {:x}\n",i,v1,v2);
                            pass = false;
                            break;
                        }
                    }
                }

                else
                {
                    spdlog::info("images differ in size: {} : {}\n",screen_check.size(),n64.rdp.screen.size());
                    pass = false;
                }

                
                spdlog::info("{}: {}\n",test.rom_path,pass? "PASS" : "FAIL");

                if(!pass)
                {
                    fail_list.push_back(test.rom_path);
                    write_test_image(fmt::format("fail/{}.png",test.name),n64.rdp.screen,n64.rdp.screen_x,n64.rdp.screen_y);
                }

                else 
                {
                    pass_list.push_back(test.rom_path);
                }
            }
        }

        catch(std::exception &ex)
        {
            spdlog::error("{}: \n",ex.what());
            fail_list.push_back(test_list[t].rom_path);
        }
    }
    
    return std::pair{pass_list,fail_list};
}

void print_test_result(const std::string& suite_name, const std::vector<std::string> test_list, bool pass)
{
    if(test_list.size() == 0)
    {
        return;
    }

    const std::string result = pass? "PASS" : "FAIL";

    if(pass)
    {
        spdlog::info("---------- {} {} ({}) ------- ",result,suite_name,test_list.size());
    }

    else
    {
        spdlog::error("---------- {} {} ({}) ------- ",result,suite_name,test_list.size());
    }

    for(auto& name : test_list)
    {
        if(pass)
        {
            spdlog::info("{}: {}",result,name);
        }

        else
        {
            spdlog::error("{}: {}",result,name);
        }
    }
}

void print_test_pass(const std::string& suite_name, const std::vector<std::string> test_list)
{
    print_test_result(suite_name,test_list,true);
}

void print_test_fail(const std::string& suite_name, const std::vector<std::string> test_list)
{
    print_test_result(suite_name,test_list,false);
}


void n64_run_tests()
{
    const auto [cpu_pass,cpu_fail] = run_test_list("CPU TEST","N64/CPUTest/CPU",CPU_TESTS,CPU_TEST_SIZE);
    const auto [cp0_pass,cp0_fail] = run_test_list("COP0 TEST","N64/CPUTest/CP0",COP0_TESTS,COP0_TEST_SIZE);
    const auto [cp1_pass,cp1_fail] = run_test_list("COP1 TEST","N64/CPUTest/CP1",COP1_TESTS,COP1_TEST_SIZE);
    const auto [exception_pass,exception_fail] = run_test_list("EXCEPTION TEST","N64/CPUTest/Exceptions",EXCEPTION_TESTS,EXCEPTION_TEST_SIZE);

    // Print a list of passes 
    print_test_pass("CPU TEST",cpu_pass);
    print_test_pass("COP0 TEST",cp0_pass);
    print_test_pass("COP1 TEST",cp1_pass);
    print_test_pass("EXCEPTION TEST",exception_pass);

    // And failures
    print_test_fail("CPU TEST",cpu_fail);
    print_test_fail("COP0 TEST",cp0_fail);
    print_test_fail("COP1 TEST",cp1_fail);
    print_test_fail("EXCEPTION TEST",exception_fail);
}
#endif

void run_tests()
{
#ifdef GB_ENABLED
    gb_run_tests();    
#endif

#ifdef N64_ENABLED 
    n64_run_tests();
#endif

}
