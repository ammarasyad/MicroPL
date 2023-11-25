#ifndef MMIO_CLI_MAIN_H
#define MMIO_CLI_MAIN_H

#include "version.h"

#ifdef NDEBUG
#define LOG(str) do {} while (0);
#else
#define LOG(str) std::cout << str << std::endl;
#endif

// Just for readability I guess?
#define ERR(str) std::cout << str << std::endl;
#define INFO(str) std::cout << str << std::endl;

#define minPL 5
#define maxPL 255

#define PL1 0xFED159A0
#define PL2 0xFED159A4

#define IA32_HWP_REQUEST 0x00000774 // MSR, bits [24:31] are the EPP value

#define maxEPP 255

#define LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#define LOW_IND(x,part_type)   0
#define DWORDn(x, n)  (*((DWORD*)&(x)+n))
#define HIDWORD(x) DWORDn(x,HIGH_IND(x,DWORD))
#define __PAIR64__(high, low) (((uint64_t) (high) << 32) | (low))

const std::string help = "\n"
"CPU MMIO Register Application v" + std::to_string(MMIO_VERSION_MAJOR) + "." + std::to_string(MMIO_VERSION_MINOR) + "." + std::to_string(MMIO_VERSION_PATCH) + "\n"
  "Usage: mmio [OPTION]...\n"
  " -h, --help                      display this help and exit\n"
  " -l, --power-limit-1 <number>    set the power limit 1 register in watts. range: 5 - 255\n"
  " -s, --power-limit-2 <number>    (optional) set the power limit 2 register in watts. range: 5 - 255\n"
  " -e, --epp <number>              (optional) set the energy performance preference register. range: 0 - 255\n";

constexpr option long_options[] = {
        {"power-limit-1", required_argument, nullptr, 'l'},
        {"power-limit-2", required_argument, nullptr, 's'},
        {"epp", required_argument, nullptr, 'e'},
        {nullptr, 0, nullptr, 0}
};

#endif
