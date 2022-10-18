#include <iostream>
#include <getopt.h>
#include <filesystem>
#include <handleapi.h>
#include <fileapi.h>
#include <ioapiset.h>
#include <winsvc.h>
#include <windows.h>
#include "main.h"

class RwDrv {
public:
    RwDrv() {
        h = CreateFileA(R"(\\.\RwDrv)", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            std::cout << "Failed to open driver handle" << std::endl;
            exit(1);
        }
    }

    ~RwDrv() {
        CloseHandle(h);
    }

    void readMem(uint64_t address, uint64_t *buffer, DWORD size) {
        PhysRw_t A;
        A.physicalAddress = address;
        A.size = size;
        A.unknown = 0;
        A.address = (uint64_t) buffer;
        DeviceIoControl(h, 0x222808, &A, sizeof(A), &A, sizeof(A), nullptr, nullptr);
    }

    void writeMem(uint64_t address, uint64_t *buffer, DWORD size) {
        PhysRw_t A;
        A.physicalAddress = address;
        A.size = size;
        A.unknown = 0;
        A.address = (uint64_t) buffer;
        DeviceIoControl(h, 0x22280C, &A, sizeof(A), nullptr, 0, nullptr, nullptr);
    }

private:
    HANDLE h;
};

BOOL wasRunning = false;

void LoadDriver() {
    BOOL fail = false;
    BOOL bService;
    char path[MAX_PATH];
    GetFullPathName("RwDrv.sys", MAX_PATH, path, nullptr);
    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    SC_HANDLE hService = nullptr;

    if (hSCM == nullptr) {
        LOG("OpenSCManager failed. Error: " << GetLastError());
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
                LOG("OpenService() failed. Error: " << GetLastError());
                fail = true;
                goto cleanup;
            }
        } else {
            LOG("CreateService() failed. Error: " << GetLastError());
        }
        hService = OpenService(hSCM, "RwDrv", SERVICE_ALL_ACCESS);
        if (hService == nullptr) {
            LOG("OpenService() failed. Error: " << GetLastError());
            fail = true;
            goto cleanup;
        }
    }

    bService = StartService(hService, 0, nullptr);
    if (!bService) {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
            LOG("Service already running");
            wasRunning = true;
            goto cleanup;
        } else {
            LOG("StartService() failed. Error: " << GetLastError());
            goto cleanup;
        }
    } else {
        LOG("Service started successfully");
        goto cleanup;
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
        LOG("OpenSCManager() failed. Error: " << GetLastError());
        fail = true;
        goto cleanup;
    }

    hService = OpenService(hSCM, "RwDrv", SERVICE_ALL_ACCESS);
    if (hService == nullptr) {
        LOG("OpenService() failed. Error: " << GetLastError());
        fail = true;
        goto cleanup;
    }

    if (!ControlService(hService, SERVICE_CONTROL_STOP, &status)) {
        LOG("ControlService() failed. Error: " << GetLastError());
        fail = true;
        goto cleanup;
    } else {
        LOG("Service stopped successfully");
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

int main(int argc, char *argv[]) {
    int choice;
    BOOL start = false;
    if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        std::cout << help << std::endl;
    } else {
        start = true;
        if (!isElevated()) {
            std::cout << "This program requires administrator rights. Rerun the application with administrator rights."
                      << std::endl;
            exit(1);
        }
        LoadDriver();
        auto driver = new RwDrv();
        while ((choice = getopt_long(argc, argv, "l:s:", long_options, nullptr)) != -1) {
            switch (choice) {
                case 'l': {
                    if (std::atoi(optarg) < minPL || std::atoi(optarg) > maxPL) {
                        std::cout << "Invalid power limits!" << std::endl;
                        goto done;
                    }
                    uint64_t pl1;
                    driver->readMem(PL1, &pl1, 2);
                    pl1 &= 0x3FF;
                    pl1 /= 8;
                    uint64_t pl2;
                    driver->readMem(PL2, &pl2, 2);
                    pl2 &= 0x3FF;
                    pl2 /= 8;
                    LOG("Previous power limit 1: " << pl1);
                    LOG("Setting power limit 1 to " << optarg << " W");
                    uint64_t new_pl1 = (((uint64_t) std::atoi(optarg)) * 8 | 0x8000);
                    if (pl2 <= std::atoi(optarg)) {
                        driver->writeMem(PL2, &new_pl1, 2);
                    }
                    driver->writeMem(PL1, &new_pl1, 2);
                    break;
                }
                case 's': {
                    if (std::atoi(optarg) < minPL || std::atoi(optarg) > maxPL) {
                        std::cout << "Invalid power limits!" << std::endl;
                        goto done;
                    }
                    uint64_t pl1;
                    driver->readMem(PL1, &pl1, 2);
                    pl1 &= 0x3FF;
                    pl1 /= 8;
                    uint64_t pl2;
                    driver->readMem(PL2, &pl2, 2);
                    pl2 &= 0x3FF;
                    pl2 /= 8;
                    LOG("Previous power limit 2: " << pl2);
                    LOG("Setting power limit 2 to " << optarg << " W");
                    uint64_t new_pl2 = (((uint64_t) std::atoi(optarg)) * 8 | 0x8000);
                    if (pl1 >= std::atoi(optarg)) {
                        driver->writeMem(PL1, &new_pl2, 2);
                    }
                    driver->writeMem(PL2, &new_pl2, 2);
                    break;
                }
                default:
                    std::cout << help << std::endl;
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