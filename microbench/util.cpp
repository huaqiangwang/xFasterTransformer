//
// Created by huaqiang on 24-9-11.
//

#include "util.h"
#include <cstring>
#include <err.h>
#include <sched.h>
#include <syscall.h>
#include <sys/stat.h>
#include <iostream>
extern "C" {
#include <xed/xed-interface.h>
}
void disassemble(uint64_t address, uint32_t length) {
    // Initialize the XED library
    xed_tables_init();

    // Set up the XED state for decoding
    xed_state_t dstate;
    xed_state_zero(&dstate);
    xed_state_init(&dstate, XED_MACHINE_MODE_LONG_64, XED_ADDRESS_WIDTH_64b, XED_ADDRESS_WIDTH_64b);

    // Create a buffer to hold the instructions
    const uint8_t* code = reinterpret_cast<const uint8_t*>(address);
    xed_decoded_inst_t xedd;
    char buffer[256];

    // Use a loop to decode each instruction in the buffer
    uint32_t offset = 0;
    while (offset < length) {
        xed_decoded_inst_zero_set_mode(&xedd, &dstate);
        xed_error_enum_t xed_error = xed_decode(&xedd, code + offset, length - offset);

        if (xed_error == XED_ERROR_NONE) {
            // Print the disassembled instruction
            xed_uint64_t xedcode = reinterpret_cast<xed_uint64_t>(code+offset);

            xed_format_context(XED_SYNTAX_INTEL, &xedd, buffer, sizeof(buffer), xedcode, 0, 0);
            std::cout << buffer << std::endl;

            // Move to the next instruction
            offset += xed_decoded_inst_get_length(&xedd);
        } else {
            std::cerr << "Error decoding instruction at offset " << offset << std::endl;
            break;
        }
    }
}


int pin_cpu(pid_t pid, int cpu)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);

    return syscall(__NR_sched_setaffinity, pid, sizeof(cpuset), &cpuset);
}

uint64_t get_ip_after_this(){
    uint64_t ip;
    asm("pop %rdi");
    asm("push %rdi");
    asm("mov %%rdi, %0": "=r"(ip));
    return ip;
}


