#ifndef MMIO_CLI_RWDRV_H
#define MMIO_CLI_RWDRV_H

typedef struct {
    uint64_t physicalAddress;
    DWORD size;
    DWORD unknown;
    uint64_t address;
} PhysRw_t;

typedef struct {
    DWORD low;
    DWORD unknown;
    DWORD reg;
    DWORD high;
} MSRRw_t;

class RwDrv {
public:
    RwDrv() {
        h = CreateFileA(R"(\\.\RwDrv)", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                        nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            ERR("Failed to open driver handle");
            exit(1);
        }
    }

    ~RwDrv() {
        CloseHandle(h);
    }

    void readMem(uint64_t address, const uint64_t *buffer, DWORD size) {
        PhysRw_t A;
        A.physicalAddress = address;
        A.size = size;
        A.unknown = 0;
        A.address = (uint64_t) buffer;
        DeviceIoControl(h, 0x222808, &A, sizeof(A), &A, sizeof(A), nullptr, nullptr);
    }

    void writeMem(uint64_t address, const uint64_t *buffer, DWORD size) {
        PhysRw_t A;
        A.physicalAddress = address;
        A.size = size;
        A.unknown = 0;
        A.address = (uint64_t) buffer;
        DeviceIoControl(h, 0x22280C, &A, sizeof(A), nullptr, 0, nullptr, nullptr);
    }

    void readMSR(int reg, uint64_t& value) {
        MSRRw_t A;
        A.reg = reg;
        A.high = 0;
        A.low = 0;

        DeviceIoControl(h, 0x222848, &A, sizeof(A), &A, sizeof(A), nullptr, nullptr);
        value = __PAIR64__(A.high, A.low);
    }

    void writeMSR(int reg, uint64_t value) {
        MSRRw_t A;
        A.reg = reg;
        A.low = (DWORD) value;
        A.high = HIDWORD(value);

        DeviceIoControl(h, 0x22284C, &A, sizeof(A), &A, sizeof(A), nullptr, nullptr);
    }

private:
    HANDLE h;
};

#endif
