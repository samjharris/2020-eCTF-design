#ifndef UTIL_H
#define UTIL_H
#include "xaxidma.h"
#include "xintc.h"
#include "constants.h"

void enableLED(u32* led);
void setLED(u32* led, struct color c);
int SetUpInterruptSystem(XIntc *XIntcInstancePtr, XInterruptHandler hdlr);
u32 fnAudioPlay(XAxiDma AxiDma, u32 offset, u32 u32NrSamples);
void fnAudioExport(XAxiDma *AxiDma_ptr, void* dest_ptr, void* src_ptr, u32 len);
XStatus fnConfigDma(XAxiDma *AxiDma);

u32 XAxiDma_Halted(XAxiDma *InstancePtr, int Direction);
u32 XAxiDma_Transceiving(XAxiDma *InstancePtr, int Direction);

#endif
