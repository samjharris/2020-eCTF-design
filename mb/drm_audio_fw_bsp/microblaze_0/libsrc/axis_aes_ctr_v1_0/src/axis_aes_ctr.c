#include "axis_aes_ctr.h"
#include "xil_io.h"

void write_key(UINTPTR base_addr, u8* key_arr) {
    for (u8 i=0; i<8; i++) {
        u32 value_to_write = 0;
        for (u8 j=0; j<=3; j++) {
            value_to_write+=(key_arr[4*i+j] << 8*(3-j));
        }
        Xil_Out32LE(base_addr+KEY_ADDR_OFFSET+4*i, value_to_write);
    }
}

void write_ctr(UINTPTR base_addr, u8* ctr_arr) {
    for (u8 i=0; i<4; i++) {
        u32 value_to_write = 0;
        for (u8 j=0; j<=3; j++) {
            value_to_write+=(ctr_arr[4*i+j] << 8*(3-j));
        }
        Xil_Out32LE(base_addr+CTR_ADDR_OFFSET+4*i, value_to_write);
    }
}

u32 get_status(UINTPTR base_addr) {
    return Xil_In32BE(base_addr+STATUS_ADDR_OFFSET);
}

u8 get_key_ready(UINTPTR base_addr) {
    return (u8) ((get_status(base_addr) && 0xff00) >> 8);
}

u8 get_ctr_ready(UINTPTR base_addr) {
    return (u8) ((get_status(base_addr) && 0xff) >> 0);
}

void write_control(UINTPTR base_addr, u8 refresh_key, u8 refresh_ctr) {
    u32 value_to_write = 0;
    if (refresh_key != 0) {
        value_to_write += 1 << 24;
    }
    if (refresh_ctr != 0) {
        value_to_write += 1 << 16;
    }
    Xil_Out32LE(base_addr+CTRL_ADDR_OFFSET, value_to_write);
}