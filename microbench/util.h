//
// Created by huaqiang on 24-9-11.
//

#ifndef DEPENDENCY_UTIL_H
#define DEPENDENCY_UTIL_H

#include <unistd.h>
#include <cstdint>

int pin_cpu(pid_t pid, int cpu);

uint64_t get_ip_after_this();
void disassemble(uint64_t p, uint32_t len);

#endif //DEPENDENCY_UTIL_H

