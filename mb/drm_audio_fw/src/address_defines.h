#ifndef SRC_ADDRESS_DEFINES_H_
#define SRC_ADDRESS_DEFINES_H_

#include "xparameters.h"

// peripheral addresses
#define DMA_MM2S_ADDR XPAR_MB_DMA_MM2S_AXI_BRAM_CTRL_0_S_AXI_BASEADDR
#define FIFO_COUNT_ADDR (XPAR_AXI_GPIO_FIFO_RNG_0_BASEADDR)
#define TRNG_READ_ADDR (XPAR_AXI_GPIO_FIFO_RNG_0_BASEADDR+0x0008)

// LED colors and controller (placed here to avoid other header include issues)
struct color {
    u32 r;
    u32 g;
    u32 b;
};

// printing utility
#define MB_PROMPT "\r\nMB> "
#define mb_printf(...) xil_printf(MB_PROMPT __VA_ARGS__)

#endif /* SRC_ADDRESS_DEFINES_H_ */
