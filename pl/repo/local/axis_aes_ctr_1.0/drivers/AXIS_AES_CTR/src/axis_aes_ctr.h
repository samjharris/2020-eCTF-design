#ifndef AXIS_AES_CTR_H
#define AXIS_AES_CTR_H

#include "xil_types.h"

#define KEY_ADDR_OFFSET 0
#define CTR_ADDR_OFFSET 32
#define STATUS_ADDR_OFFSET 48
#define CTRL_ADDR_OFFSET 52

void write_key(UINTPTR base_addr, u8* key_arr);
void write_ctr(UINTPTR base_addr, u8* ctr_arr);
void read_ctr(UINTPTR base_addr, u8* ctr_arr);
u32 get_status(UINTPTR base_addr);
u8 get_key_ready(UINTPTR base_addr);
u8 get_ctr_ready(UINTPTR base_addr);
void write_control(UINTPTR base_addr, u8 refresh_key, u8 refresh_ctr);

#endif