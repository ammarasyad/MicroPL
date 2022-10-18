//
// Created by ammar on 18/10/2022.
//
#ifndef MMIO_CLI_MAIN_H
#define MMIO_CLI_MAIN_H

//#define PL1 ((*((uint32_t volatile *) 0xFED159A0)) & 0x3FFF)
//#define PL2 ((*((uint32_t volatile *) 0xFED159A4)) & 0x3FFF)

#include <minwindef.h>

#define minPL 5
#define maxPL 255

#define PL1 0xFED159A0
#define PL2 0xFED159A4

typedef struct PhysRw_t {
    uint64_t physicalAddress;
    DWORD size;
    DWORD unknown;
    uint64_t address;
} PhysRw_t;

const char* help = R"(
CPU MMIO Register Application
  Usage: mmio [OPTION]...
  /h, --help                      display this help and exit
  /l, --power-limit-1 <number>    set the power limit 1 register in watts. range: 5 - 255
  /s, --power-limit-2 <number>    (optional) set the power limit 2 register in watts. range: 5 - 255)";

const struct option long_options[] = {
        {"power-limit-1", required_argument, nullptr, 'l'},
        {"power-limit-2", required_argument, nullptr, 's'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
};

#endif //MMIO_CLI_MAIN_H