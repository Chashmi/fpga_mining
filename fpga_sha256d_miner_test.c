#include "xparameters.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "sleep.h"

#define SHA256D_BASE       0x43C00000

#define REG_RESET          0x00
#define REG_START          0x04
#define REG_STATUS         0x08
#define REG_GOLDEN_NONCE   0x0C
#define REG_CUR_REQ        0x10
#define REG_CUR_NONCE      0x14

#define REG_MID(i)         (0x100 + ((i) * 4))
#define REG_RES(i)         (0x200 + ((i) * 4))
#define REG_TGT(i)         (0x300 + ((i) * 4))

static inline void write_reg(u32 offset, u32 value) {
    xil_printf("Write to offset 0x%03X: 0x%08X\n", offset, value);
    Xil_Out32(SHA256D_BASE + offset, value);
}

static inline u32 read_reg(u32 offset) {
    u32 val = Xil_In32(SHA256D_BASE + offset);
    xil_printf("Read from offset 0x%03X: 0x%08X\n", offset, val);
    return val;
}

void write_block_header(const u32 mid[8], const u32 res[3], const u32 tgt[8]) {
    xil_printf("Writing Midstate...\n");
    for (int i = 0; i < 8; ++i) write_reg(REG_MID(i), mid[i]);

    xil_printf("Writing Residual...\n");
    for (int i = 0; i < 3; ++i) write_reg(REG_RES(i), res[i]);

    xil_printf("Writing Target...\n");
    for (int i = 0; i < 8; ++i) write_reg(REG_TGT(i), tgt[i]);
}

int main() {
    init_platform();
    xil_printf("== SHA256D FPGA Test Start ==\n");

    u32 mid[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    u32 res[3] = { 0x80000000, 0x00000000, 0x00000100 };
    u32 tgt[8] = {
        0x0000ffff, 0xffffffff, 0xffffffff, 0xffffffff,
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
    };

    // 1. Load midstate, residual, target
    write_block_header(mid, res, tgt);

    // 2. Reset core
    xil_printf("Sending reset pulse...\n");
    write_reg(REG_RESET, 1);
    usleep(1000);
    write_reg(REG_RESET, 0);

    // 3. Start hashing
    xil_printf("Sending start signal...\n");
    write_reg(REG_START, 1);

    // 4. Poll status
    while (1) {
        u32 status = read_reg(REG_STATUS);
        if (status & 0x01) {
            xil_printf("Golden Nonce Found!\n");
            u32 golden = read_reg(REG_GOLDEN_NONCE);
            xil_printf("Golden Nonce: 0x%08X\n", golden);
            break;
        } else if (status & 0x02) {
            xil_printf("Nonce not found in range.\n");
            break;
        }
        u32 cur_nonce = read_reg(REG_CUR_NONCE);
        xil_printf("Current Nonce: 0x%08X\n", cur_nonce);
        usleep(10000);
    }

    xil_printf("== SHA256D FPGA Test End ==\n");
    cleanup_platform();
    return 0;
}
