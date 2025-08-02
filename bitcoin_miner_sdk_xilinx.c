/*
 * Bitcoin Miner SDK for Zybo Z-10 - Xilinx SDK Version
 * Interfaces with FPGA SHA-256 miner
 * 
 * Base Address: 0x43C00000
 * Register Map:
 * Bank 0: Control registers
 * Bank 1: MID_STATE (8 x 32-bit)
 * Bank 2: RESIDUAL_DATA (3 x 32-bit)
 * Bank 3: TARGET (8 x 32-bit)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "xil_io.h"
#include "xil_types.h"
#include "xparameters.h"

// Base address from block design
#define MINER_BASE_ADDR 0x43C00000

// Register bank offsets
#define BANK_0_OFFSET 0x0000  // Control registers
#define BANK_1_OFFSET 0x0100  // MID_STATE
#define BANK_2_OFFSET 0x0200  // RESIDUAL_DATA  
#define BANK_3_OFFSET 0x0300  // TARGET

// Control register offsets
#define CTRL_SRST            0x0000
#define CTRL_START           0x0004
#define CTRL_CURRENT_HASH_REQ 0x0010

// Status register offsets (read-only)
#define STATUS_FOUND         0x0000
#define STATUS_NOT_FOUND     0x0004
#define STATUS_GOLDEN_NONCE  0x0008
#define STATUS_CURRENT_NONCE 0x000C

// Bitcoin block header structure
typedef struct {
    uint32_t version;
    uint8_t prev_block[32];
    uint8_t merkle_root[32];
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} bitcoin_block_header_t;

// Mining parameters
typedef struct {
    uint32_t mid_state[8];      // SHA-256 mid-state
    uint32_t residual_data[3];  // Remaining block data
    uint32_t target[8];         // Difficulty target
} mining_params_t;

// Xilinx SDK memory-mapped I/O functions
void write_register(uint32_t offset, uint32_t value) {
    Xil_Out32(MINER_BASE_ADDR + offset, value);
    printf("Write: 0x%08X = 0x%08X\n", MINER_BASE_ADDR + offset, value);
}

uint32_t read_register(uint32_t offset) {
    uint32_t value = Xil_In32(MINER_BASE_ADDR + offset);
    printf("Read: 0x%08X = 0x%08X\n", MINER_BASE_ADDR + offset, value);
    return value;
}

// Write MID_STATE to FPGA
void write_mid_state(uint32_t* mid_state) {
    printf("Writing MID_STATE...\n");
    for (int i = 0; i < 8; i++) {
        write_register(BANK_1_OFFSET + (i * 4), mid_state[i]);
        printf("  MID_STATE[%d] = 0x%08X\n", i, mid_state[i]);
    }
}

// Write RESIDUAL_DATA to FPGA
void write_residual_data(uint32_t* residual_data) {
    printf("Writing RESIDUAL_DATA...\n");
    for (int i = 0; i < 3; i++) {
        write_register(BANK_2_OFFSET + (i * 4), residual_data[i]);
        printf("  RESIDUAL_DATA[%d] = 0x%08X\n", i, residual_data[i]);
    }
}

// Write TARGET to FPGA
void write_target(uint32_t* target) {
    printf("Writing TARGET...\n");
    for (int i = 0; i < 8; i++) {
        write_register(BANK_3_OFFSET + (i * 4), target[i]);
        printf("  TARGET[%d] = 0x%08X\n", i, target[i]);
    }
}

// Start mining
void start_mining(void) {
    printf("Starting mining...\n");
    write_register(CTRL_START, 1);
}

// Stop mining
void stop_mining(void) {
    printf("Stopping mining...\n");
    write_register(CTRL_SRST, 1);
    usleep(1000); // Small delay
    write_register(CTRL_SRST, 0);
}

// Check if golden nonce was found
int check_found(void) {
    uint32_t found = read_register(STATUS_FOUND);
    return (found & 0x1);
}

// Get golden nonce if found
uint32_t get_golden_nonce(void) {
    return read_register(STATUS_GOLDEN_NONCE);
}

// Get current nonce being processed
uint32_t get_current_nonce(void) {
    write_register(CTRL_CURRENT_HASH_REQ, 1);
    usleep(1000); // Small delay for CDC
    write_register(CTRL_CURRENT_HASH_REQ, 0);
    return read_register(STATUS_CURRENT_NONCE);
}

// Print current mining status
void print_mining_status(void) {
    uint32_t current_nonce = get_current_nonce();
    uint32_t found = read_register(STATUS_FOUND);
    uint32_t not_found = read_register(STATUS_NOT_FOUND);
    
    printf("=== Mining Status ===\n");
    printf("Current Nonce: 0x%08X (%u)\n", current_nonce, current_nonce);
    printf("Found: %s\n", found ? "YES" : "NO");
    printf("Not Found: %s\n", not_found ? "YES" : "NO");
    
    if (found) {
        uint32_t golden_nonce = get_golden_nonce();
        printf("Golden Nonce: 0x%08X (%u)\n", golden_nonce, golden_nonce);
    }
    printf("===================\n");
}

// Set a very easy difficulty for testing (will find nonces quickly)
void set_test_difficulty(void) {
    uint32_t easy_target[8] = {
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x000000FF  // Very easy target
    };
    write_target(easy_target);
    printf("Set test difficulty (very easy target)\n");
}

// Set real Bitcoin difficulty (will rarely find nonces)
void set_real_difficulty(void) {
    // Current Bitcoin difficulty target (as of 2024)
    uint32_t real_target[8] = {
        0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000  // Very hard target
    };
    write_target(real_target);
    printf("Set real Bitcoin difficulty (very hard target)\n");
}

// Process Bitcoin block header and prepare mining parameters
void process_block_header(bitcoin_block_header_t* header, mining_params_t* params) {
    printf("Processing block header...\n");
    printf("Version: 0x%08X\n", header->version);
    printf("Timestamp: %u\n", header->timestamp);
    printf("Bits: 0x%08X\n", header->bits);
    printf("Nonce: 0x%08X\n", header->nonce);
    
    // For demonstration, we'll use simplified parameters
    // In a real implementation, you would:
    // 1. Calculate the SHA-256 mid-state from the first 512 bits of the header
    // 2. Extract the residual data (remaining bits)
    // 3. Convert the difficulty bits to a target
    
    // Simplified test parameters
    for (int i = 0; i < 8; i++) {
        params->mid_state[i] = 0x12345678 + i;  // Test mid-state
    }
    
    params->residual_data[0] = header->nonce;  // Start with the nonce
    params->residual_data[1] = 0x80000000;     // Padding
    params->residual_data[2] = 0x00000140;     // Length
    
    // Convert difficulty bits to target
    uint32_t exp = (header->bits >> 24) & 0xFF;
    uint32_t mantissa = header->bits & 0xFFFFFF;
    
    // Simplified target calculation
    for (int i = 0; i < 8; i++) {
        params->target[i] = 0xFFFFFFFF;
    }
    params->target[0] = mantissa;
    
    printf("Mining parameters prepared\n");
}

// Main mining loop with easy difficulty for testing
void mining_loop_test(void) {
    printf("Starting Bitcoin mining loop (TEST MODE - Easy Difficulty)...\n");
    
    // Initialize with test parameters
    bitcoin_block_header_t header = {
        .version = 0x20000000,
        .timestamp = (uint32_t)time(NULL),
        .bits = 0x1D00FFFF,  // Very easy difficulty
        .nonce = 0
    };
    
    mining_params_t params;
    process_block_header(&header, &params);
    
    // Set test difficulty for demonstration
    set_test_difficulty();
    
    // Write parameters to FPGA
    write_mid_state(params.mid_state);
    write_residual_data(params.residual_data);
    
    // Start mining
    start_mining();
    
    int iteration = 0;
    while (1) {
        // Check status every second
        if (iteration % 10 == 0) {
            print_mining_status();
        }
        
        // Check if found
        if (check_found()) {
            printf("\n GOLDEN NONCE FOUND! \n");
            uint32_t golden_nonce = get_golden_nonce();
            printf("Golden Nonce: 0x%08X (%u)\n", golden_nonce, golden_nonce);
            
            // Stop mining
            stop_mining();
            break;
        }
        
        // Check if not found (reached end of nonce range)
        uint32_t not_found = read_register(STATUS_NOT_FOUND);
        if (not_found) {
            printf("\n No nonce found in current range\n");
            stop_mining();
            break;
        }
        
        usleep(100000); // 100ms delay
        iteration++;
        
        // Safety check - stop after 1000 iterations (100 seconds)
        if (iteration > 1000) {
            printf("\n Timeout reached, stopping mining\n");
            stop_mining();
            break;
        }
    }
    
    printf("Mining loop completed\n");
}

// Main mining loop with real difficulty (for observation only)
void mining_loop_real(void) {
    printf("Starting Bitcoin mining loop (REAL MODE - Real Difficulty)...\n");
    printf("Note: This will likely never find a nonce with current difficulty!\n");
    
    // Initialize with test parameters
    bitcoin_block_header_t header = {
        .version = 0x20000000,
        .timestamp = (uint32_t)time(NULL),
        .bits = 0x1703FFFC,  // Current Bitcoin difficulty
        .nonce = 0
    };
    
    mining_params_t params;
    process_block_header(&header, &params);
    
    // Set real difficulty
    set_real_difficulty();
    
    // Write parameters to FPGA
    write_mid_state(params.mid_state);
    write_residual_data(params.residual_data);
    
    // Start mining
    start_mining();
    
    int iteration = 0;
    while (1) {
        // Check status every 10 seconds
        if (iteration % 100 == 0) {
            print_mining_status();
        }
        
        // Check if found (unlikely!)
        if (check_found()) {
            printf("\n GOLDEN NONCE FOUND! \n");
            printf("This is extremely unlikely with real difficulty!\n");
            uint32_t golden_nonce = get_golden_nonce();
            printf("Golden Nonce: 0x%08X (%u)\n", golden_nonce, golden_nonce);
            
            // Stop mining
            stop_mining();
            break;
        }
        
        // Check if not found (reached end of nonce range)
        uint32_t not_found = read_register(STATUS_NOT_FOUND);
        if (not_found) {
            printf("\n No nonce found in current range\n");
            stop_mining();
            break;
        }
        
        usleep(100000); // 100ms delay
        iteration++;
        
        // Safety check - stop after 10000 iterations (1000 seconds)
        if (iteration > 10000) {
            printf("\n Timeout reached, stopping mining\n");
            printf("This demonstrates that real Bitcoin difficulty is extremely high!\n");
            stop_mining();
            break;
        }
    }
    
    printf("Mining loop completed\n");
}

// Main function
int main(void) {
    printf("=== Bitcoin Miner SDK for Zybo Z-10 ===\n");
    printf("Base Address: 0x%08X\n", MINER_BASE_ADDR);
    printf("Starting mining demonstration...\n\n");
    
    // Initialize FPGA
    stop_mining(); // Ensure clean state
    
    // Ask user which mode to run
    printf("Choose mining mode:\n");
    printf("1. Test mode (easy difficulty - will find nonces)\n");
    printf("2. Real mode (real difficulty - for observation only)\n");
    printf("Enter choice (1 or 2): ");
    
    int choice;
    scanf("%d", &choice);
    
    if (choice == 1) {
        mining_loop_test();
    } else if (choice == 2) {
        mining_loop_real();
    } else {
        printf("Invalid choice, running test mode...\n");
        mining_loop_test();
    }
    
    printf("\nMining demonstration completed\n");
    return 0;
} 