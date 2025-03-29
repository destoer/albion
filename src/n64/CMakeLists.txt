

add_library(n64 STATIC "src/n64.cpp")
target_compile_definitions(n64 PUBLIC -DN64_ENABLED)
target_include_directories(n64 PUBLIC "headers/")
target_include_directories(n64 PUBLIC "src/")

# handle opcode generation
set(N64_OPCODE_TABLE "${CMAKE_SOURCE_DIR}/src/n64/src/instr/mips_instr_table.h")

if(WIN32)
add_custom_command(
    OUTPUT ${N64_OPCODE_TABLE}
    COMMAND ${CMAKE_SOURCE_DIR}/beyond-all-repair/beyond_all_repair -g 3 ${N64_OPCODE_TABLE} 
    DEPENDS ${CMAKE_SOURCE_DIR}/beyond-all-repair/src/mips_table.inl
)
else ()
add_custom_command(
    OUTPUT ${N64_OPCODE_TABLE}
    COMMAND ${CMAKE_SOURCE_DIR}/beyond-all-repair/beyond_all_repair -g 3 ${N64_OPCODE_TABLE} 
    DEPENDS ${CMAKE_SOURCE_DIR}/beyond-all-repair/src/mips_table.inl
)
endif ()	

target_link_libraries(n64 spdlog bar)