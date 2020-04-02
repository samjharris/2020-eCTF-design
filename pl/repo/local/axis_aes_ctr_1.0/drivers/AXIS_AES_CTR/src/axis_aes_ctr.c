#include "axis_aes_ctr.h"
#include "xil_io.h"

void aes_write_key(UINTPTR base_addr, u8* key_arr) {
    for (u8 i=0; i<8; i++) {
        u32 value_to_write = 0;
        for (u8 j=0; j<=3; j++) {
            value_to_write+=(key_arr[4*i+j] << 8*(3-j));
        }
        Xil_Out32LE(base_addr+KEY_ADDR_OFFSET+4*i, value_to_write);
    }
}

void aes_write_ctr(UINTPTR base_addr, u8* ctr_arr) {
    for (u8 i=0; i<4; i++) {
        u32 value_to_write = 0;
        for (u8 j=0; j<=3; j++) {
            value_to_write+=(ctr_arr[4*i+j] << 8*(3-j));
        }
        Xil_Out32LE(base_addr+CTR_ADDR_OFFSET+4*i, value_to_write);
    }
}

void aes_read_ctr(UINTPTR base_addr, u8* ctr_arr) {
    u32 io_arr[4];
    for (u8 i=0; i<4; i++) {
        io_arr[i]=Xil_In32LE(base_addr+CTR_ADDR_OFFSET+4*i);
    }
    for (u8 i=0; i<4; i++) {
        for (u8 j=0; j<4; j++) {
            ctr_arr[4*i+(3-j)]= (u8) ((io_arr[i] & (0xff << 8*j)) >> 8*j);
        }
    }
}

u32 aes_get_status(UINTPTR base_addr) {
    return Xil_In32LE(base_addr+STATUS_ADDR_OFFSET);
}

u8 aes_get_key_ready(UINTPTR base_addr) {
    return (u8) ((aes_get_status(base_addr) & 0xff) >> 0);
}

u8 aes_get_ctr_ready(UINTPTR base_addr) {
    return (u8) ((aes_get_status(base_addr) & 0xff00) >> 8);
}

u8 aes_get_tdest(UINTPTR base_addr) {
    return (u8) ((aes_get_status(base_addr) & 0xff0000) >> 16);
}
void aes_set_tdest(UINTPTR base_addr, u8 tdest) {
    Xil_Out8(base_addr+CTRL_ADDR_OFFSET+1, tdest);
}

void aes_write_control(UINTPTR base_addr, u8 refresh_key, u8 refresh_ctr) {
    u32 value_to_write = 0;
    if (refresh_key != 0) {
        value_to_write += 1 << 0;
    }
    if (refresh_ctr != 0) {
        value_to_write += 1 << 8;
    }
    Xil_Out16LE(base_addr+CTRL_ADDR_OFFSET+2, value_to_write);
}
