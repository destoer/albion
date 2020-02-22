#pragma once
#include "cpu.h"
#include "memory.h"
#include "disass.h"
#include "display.h"
#include <destoer-emu/debug.h>


enum class button 
{
    a = 0,b=1,select=2,start=3,
    right=4,left=5,up=6,down=7,
    r=8,l=9
};

class GBA
{
public:
    void reset(std::string filename);
    void run();
    
    
    //void handle_input();
    void button_event(button b, bool down);

    Cpu cpu;
    Mem mem;
    Disass disass;
    Display disp;
    Debug debug;

    bool quit = false;
};
