#include <iostream>
#include <getopt.h>
#include <filesystem>
#include <handleapi.h>
#include <fileapi.h>
#include <winsvc.h>
#include <windows.h>
#include <bitset>
#include <thread>
#include "main.h"
#include "rwdrv.h"
#include "version.h"

BOOL wasRunning = false;

void LoadDriver() {
    BOOL fail = false;
    BOOL bService;
    char path[MAX_PATH];
    GetFullPathName("RwDrv.sys", MAX_PATH, path, nullptr);
    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = nullptr;

    if (hSCM == nullptr) {
        LOG("OpenSCManager failed. Error: " << GetLastError())
        fail = true;
        goto cleanup;
    }

    hService = CreateService(hSCM,
                             "RwDrv",
                             "RwDrv",
                             SERVICE_ALL_ACCESS,
                             SERVICE_KERNEL_DRIVER,
                             SERVICE_DEMAND_START,
                             SERVICE_ERROR_IGNORE,
                             path,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr);

    if (hService == nullptr) {
        if (GetLastError() == ERROR_SERVICE_EXISTS) {
            hService = OpenService(hSCM, "RwDrv", SERVICE_ALL_ACCESS);
            if (hService == nullptr) {
                LOG("OpenService() failed. Error: " << GetLastError())
                fail = true;
                goto cleanup;
            }
        } else {
            LOG("CreateService() failed. Error: " << GetLastError())
            fail = true;
            goto cleanup;
        }
    }

    bService = StartService(hService, 0, nullptr);
    if (!bService) {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
            LOG("Service already running")
            wasRunning = true;
        } else {
            LOG("StartService() failed. Error: " << GetLastError())
        }
    } else {
        LOG("Service started successfully")
    }

    cleanup:
    if (hSCM) {
        CloseHandle(hSCM);
    }
    if (hService) {
        CloseServiceHandle(hService);
    }
    if (fail) {
        exit(1);
    }
}

void UnloadDriver() {
    BOOL fail = false;
    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = nullptr;
    SERVICE_STATUS status;

    if (hSCM == nullptr) {
        LOG("OpenSCManager() failed. Error: " << GetLastError())
        fail = true;
        goto cleanup;
    }

    hService = OpenService(hSCM, "RwDrv", SERVICE_ALL_ACCESS);
    if (hService == nullptr) {
        LOG("OpenService() failed. Error: " << GetLastError())
        fail = true;
        goto cleanup;
    }

    if (!ControlService(hService, SERVICE_CONTROL_STOP, &status)) {
        LOG("ControlService() failed. Error: " << GetLastError())
        fail = true;
    } else {
        LOG("Service stopped successfully")
    }

    cleanup:
    if (hSCM) {
        CloseHandle(hSCM);
    }
    if (hService) {
        CloseServiceHandle(hService);
    }
    if (fail) {
        exit(1);
    }
}

BOOL isElevated() {
    BOOL fRet = FALSE;
    HANDLE hToken = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    return fRet;
}

uint64_t checkIfValid(char* str) {
    char* endPtr = nullptr;
    auto pl = (uint64_t) std::strtol(str, &endPtr, 10);
    if (endPtr == str) {
        ERR("Invalid number!")
        exit(1);
    }
    return pl;
}

uint64_t readPL(RwDrv* driver, uint32_t address) {
    uint64_t pl;
    driver->readMem(address, &pl, 2);
    return (pl & 0x3FF) >> 3;
}

void writePL(RwDrv* driver, uint32_t address, uint64_t value) {
    value = (value * 8) | 0x8000;
    driver->writeMem(address, &value, 2);
}

uint64_t readEPP(RwDrv* driver, int reg) {
    uint64_t msr = 0;
    driver->readMSR(reg, &msr);
    return msr;
}

void writeEPP(RwDrv* driver, int reg, uint64_t value) {
    const unsigned int core = std::thread::hardware_concurrency();
    HANDLE hThread = GetCurrentThread();
    for (int i = 0; i < core; i++) {
        SetThreadAffinityMask(hThread, 1 << i);
        driver->writeMSR(reg, value);
    }
}

int main(int argc, char *argv[]) {
    int choice;
    BOOL start = false;
    if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        INFO(help)
    } else {
        start = true;
        if (!isElevated()) {
            ERR("This program requires administrator rights. Rerun the application with administrator rights.")
            exit(1);
        }
        LoadDriver();
        auto driver = new RwDrv();
        while ((choice = getopt_long(argc, argv, "l:s:e:", long_options, nullptr)) != -1) {
            uint64_t argValue = checkIfValid(optarg);
            switch (choice) {
                case 'l': {
                    if (argValue < minPL || argValue > maxPL) {
                        ERR("Invalid power limits!")
                        goto done;
                    }
                    uint64_t pl1 = readPL(driver, PL1);
                    uint64_t pl2 = readPL(driver, PL2);
                    LOG("Previous power limit 1: " << pl1)
                    LOG("Setting power limit 1 to " << argValue << " W")
                    if (pl2 <= argValue) {
                        writePL(driver, PL2, argValue);
                    }
                    writePL(driver, PL1, argValue);
                    break;
                }
                case 's': {
                    if (argValue < minPL || argValue > maxPL) {
                        ERR("Invalid power limits!")
                        goto done;
                    }
                    uint64_t pl1 = readPL(driver, PL1);
                    uint64_t pl2 = readPL(driver, PL2);
                    LOG("Previous power limit 2: " << pl2)
                    LOG("Setting power limit 2 to " << argValue << " W")
                    if (pl1 >= argValue) {
                        writePL(driver, PL1, argValue);
                    }
                    writePL(driver, PL2, argValue);
                    break;
                }
                case 'e': {
                    if (argValue > maxEPP) {
                        ERR("Invalid EPP!")
                        goto done;
                    }
                    uint64_t value = readEPP(driver, IA32_HWP_REQUEST);
                    LOG("Previous EPP: " << ((value & 0xFF000000) >> 24))
                    value = ((value & ~(uint64_t) 0xFF000000) | argValue << 24);
                    LOG("Setting EPP to " << argValue);
                    writeEPP(driver, IA32_HWP_REQUEST, value);
                    break;
                }
                default:
                    INFO(help)
                    goto done;
            }
        }
    }
    done:
    if (start && !wasRunning) {
        UnloadDriver();
    }
    return 0;
}