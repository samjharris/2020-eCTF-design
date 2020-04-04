#include "util.h"
#include "constants.h"
#include "PWM.h"

/*
 * This function enables the PWM module and sets its period so it can drive the RGB LED
 */
void enableLED(u32* led){
    PWM_Enable((u32)led);
    PWM_Set_Period((u32)led, (u32)59999);
}

/*
 * This function drives the PWM based upon the input color struct
 * It drives LD0 with the input color struct
 */
void setLED(u32* led, struct color c){
	PWM_Set_Duty((u32)led, c.b, (u32)0);
	PWM_Set_Duty((u32)led, c.g, (u32)1);
	PWM_Set_Duty((u32)led, c.r, (u32)2);
}

/******************************************************************************/
/**
*
* This function connects the interrupt handler of the interrupt controller to
* the processor.  This function is seperate to allow it to be customized for
* each application.  Each processor or RTOS may require unique processing to
* connect the interrupt handler.
*
* @param	None.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
int SetUpInterruptSystem(XIntc *XIntcInstancePtr, XInterruptHandler hdlr)
{
	int Status;


	/*
	 * Connect a device driver handler that will be called when an interrupt
	 * for the device occurs, the device driver handler performs the
	 * specific interrupt processing for the device.
	 */
	Status = XIntc_Connect(XIntcInstancePtr, XPAR_INTC_0_DEVICE_ID,
				    hdlr,
				   (void *)0);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Start the interrupt controller such that interrupts are enabled for
	 * all devices that cause interrupts, specify simulation mode so that
	 * an interrupt can be caused by software rather than a real hardware
	 * interrupt.
	 */
	Status = XIntc_Start(XIntcInstancePtr, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Enable the interrupt for the device
	 */
	XIntc_Enable(XIntcInstancePtr, XPAR_INTC_0_DEVICE_ID);

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)XIntc_InterruptHandler,
				XIntcInstancePtr);

	/*
	 * Enable exceptions.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;

}

/******************************************************************************
 * Configure the I2S controller to transmit data, which will be read out from
 * the local memory vector (Mem)
 *
 * @param	u32NrSamples is the number of samples to play.
 *
 * @return	none.
 *****************************************************************************/
u32 fnAudioPlay(XAxiDma AxiDma, u32 offset, u32 u32NrSamples)
{
	u32 status;
	// Old is XPAR_MB_DMA_AXI_BRAM_CTRL_0_S_AXI_BASEADDR
	status = XAxiDma_SimpleTransfer(&AxiDma,(u32) (DMA_MM2S_ADDR + offset), u32NrSamples, XAXIDMA_DMA_TO_DEVICE);

	return status;
}

void fnAudioExport(XAxiDma *AxiDma_ptr, void* dest_ptr, void* src_ptr, u32 len)
{
    // Do DMA (like in the play function) to read in decrypted audio data
    // TODO: DMA error handling

    /*
     * This is arbitrary as long as it is larger than the AES block size
     * (i.e. S2MM will ingest a whole packet)
     * Using this fixed constant instead of proper length bookkeeping
     * There are a number of errors, including off-by-one errors, and bugs in the AXI DMA core
     * Weirdness labeled (?!?!) is from a mixture of software and hardware bugs
     * To anyone reading this code: brownie points for figuring out what is happening
     */
    const u32 S2MM_REQUEST_SIZE=32;
    // This is signed because attempted copy below will be smaller than actual copied on last block (?!?!)
    s32 rem=len;
    u32 block_counter = 0;
    u32 dest_offset = 0;

    while (rem>0) {
        xil_printf("\r\n======== LOOP BEGIN ========\r\n");
        u32 attempted_copy_size = (rem > CHUNK_DOUT_SIZE) ? CHUNK_DOUT_SIZE : rem;

        xil_printf("0x%08x bytes remaining\r\n", rem);
        //xil_printf("Copy size is 0x%08x\r\n", attempted_copy_size);
        xil_printf("S2MM new transfer (begin) (ptr %08x, len %d)\r\n",
                            DMA_S2MM_ADDR,attempted_copy_size);

        // Start the DMA transactions in both directions
        // memcpy into transmit BRAM and zero out receive BRAM
        memcpy((void *)(DMA_MM2S_ADDR),
                           (void *)(src_ptr + dest_offset),
                           (u32)(attempted_copy_size));
        memset((void*)DMA_S2MM_ADDR, 0, attempted_copy_size);
        // Actually start transfer
        u32 s2mm_init_status = XAxiDma_SimpleTransfer(AxiDma_ptr,(u32) DMA_S2MM_ADDR, S2MM_REQUEST_SIZE, XAXIDMA_DEVICE_TO_DMA);
        u32 mm2s_init_status = XAxiDma_SimpleTransfer(AxiDma_ptr,(u32) DMA_MM2S_ADDR, attempted_copy_size, XAXIDMA_DMA_TO_DEVICE);
        xil_printf("mm2s_init_status is %d, s2mm_init_status is %d\r\n",mm2s_init_status,s2mm_init_status);

        // Convoluted stuff to handle TLAST signaling
        // AES core signals TLAST at the end of each block
        // This is super annoying
        u32 bytes_copied_this_block = 0;
        while (1) {
            /*
            xil_printf("DMA_TO_DEVICE status is %04x, DEVICE_TO_DMA status is %04x\r\n",
                XAxiDma_ReadReg(sAxiDma.RegBase +
                    XAXIDMA_TX_OFFSET,XAXIDMA_SR_OFFSET),
                XAxiDma_ReadReg(sAxiDma.RegBase +
                    XAXIDMA_RX_OFFSET,XAXIDMA_SR_OFFSET));*/
            // Wait for S2MM to finish
            while (XAxiDma_Transceiving(AxiDma_ptr, XAXIDMA_DEVICE_TO_DMA)) {}
            if (rem<=CHUNK_DOUT_SIZE || block_counter == 0) {
                xil_printf("DMA_TO_DEVICE status is %04x, DEVICE_TO_DMA status is %04x\r\n",
                            XAxiDma_ReadReg(AxiDma_ptr->RegBase +
                                XAXIDMA_TX_OFFSET,XAXIDMA_SR_OFFSET),
                            XAxiDma_ReadReg(AxiDma_ptr->RegBase +
                                XAXIDMA_RX_OFFSET,XAXIDMA_SR_OFFSET));
            }
            u32 length_read_in=XAxiDma_ReadReg(AxiDma_ptr->RxBdRing[0].ChanBase,
                XAXIDMA_BUFFLEN_OFFSET);
            bytes_copied_this_block+=length_read_in;
            if (rem<=CHUNK_DOUT_SIZE || block_counter == 0) {
                xil_printf("Read in %d bytes\r\n",length_read_in);
                xil_printf("S2MM new transfer (potential) (ptr %08x+%08x, len %d)\r\n",
                    DMA_S2MM_ADDR,bytes_copied_this_block,attempted_copy_size-bytes_copied_this_block);
            }
            if ((!XAxiDma_Transceiving(AxiDma_ptr, XAXIDMA_DMA_TO_DEVICE) && !XAxiDma_Transceiving(AxiDma_ptr, XAXIDMA_DEVICE_TO_DMA))
                    /*|| attempted_copy_size<=bytes_copied_this_block*/) {
                xil_printf("Exiting loop\r\n");
                u32 debug_remain=attempted_copy_size-bytes_copied_this_block;
                if (debug_remain!=0) {
                    xil_printf("Skipping transfer of size %d\r\n",debug_remain);
                }
                break;
            }
            XAxiDma_SimpleTransfer(AxiDma_ptr,
                (u32) (DMA_S2MM_ADDR+bytes_copied_this_block), S2MM_REQUEST_SIZE,
                XAXIDMA_DEVICE_TO_DMA);
        }
        xil_printf("DMA_TO_DEVICE status is %04x, DEVICE_TO_DMA status is %04x at block %d completion\r\n",
                        XAxiDma_ReadReg(AxiDma_ptr->RegBase +
                            XAXIDMA_TX_OFFSET,XAXIDMA_SR_OFFSET),
                        XAxiDma_ReadReg(AxiDma_ptr->RegBase +
                            XAXIDMA_RX_OFFSET,XAXIDMA_SR_OFFSET),
                        block_counter);
        if (attempted_copy_size!=bytes_copied_this_block) {
            xil_printf("Attempted to copy %d but actually copied %d\r\n",attempted_copy_size,bytes_copied_this_block);
        }
        /*xil_printf("S2MM BRAM snippet: ");
        for (u32 i=0; i<8; i++) {
            xil_printf("%08x", Xil_In32(DMA_S2MM_ADDR+4*i));
        }
        xil_printf("\r\n");*/
        xil_printf("memcpy(%08x+%08x, %08x, %08x)\r\n",dest_ptr,dest_offset,DMA_S2MM_ADDR,bytes_copied_this_block);
        memcpy(dest_ptr+dest_offset, (void*)DMA_S2MM_ADDR, bytes_copied_this_block);
        /*mb_printf("DDR RAM snippet (current=%08x): ",dest_ptr+dest_offset);
        for (u32 i=0; i<8; i++) {
            xil_printf("%08x", *((u32*)(dest_ptr+dest_offset+4*i)));
        }
        xil_printf("\r\n");*/

        rem-=bytes_copied_this_block;
        block_counter++;
        // Track dest_offset dynamically as MM2S terminates early on very first transaction(?!?!)
        dest_offset+=bytes_copied_this_block;
    }
    mb_printf("DDR RAM snippet (beginning=%08x): ",dest_ptr);
    for (u32 i=0; i<8; i++) {
        xil_printf("%08x", *((u32*)(dest_ptr+4*i)));
    }
    xil_printf("\r\n");
}

XStatus fnConfigDma(XAxiDma *AxiDma)
{
	int Status;
	XAxiDma_Config *pCfgPtr;

	//Make sure the DMA hardware is present in the project
	//Ensures that the DMA hardware has been loaded
	pCfgPtr = XAxiDma_LookupConfig(XPAR_AXIDMA_0_DEVICE_ID);
	if (!pCfgPtr)
	{

		xil_printf(MB_PROMPT "No config found for %d\r\n", XPAR_AXIDMA_0_DEVICE_ID);

		return XST_FAILURE;
	}

	//Initialize DMA
	//Reads and sets all the available information
	//about the DMA to the AxiDma variable
	Status = XAxiDma_CfgInitialize(AxiDma, pCfgPtr);
	if (Status != XST_SUCCESS)
	{

		xil_printf(MB_PROMPT "Initialization failed %d\r\n");

		return XST_FAILURE;
	}

	//Ensures that the Scatter Gather mode is not active
	if(XAxiDma_HasSg(AxiDma))
	{
		xil_printf(MB_PROMPT "Device configured as SG mode\r\n");

		return XST_FAILURE;
	}

	// Ensure that both directions are available
	if(!AxiDma->HasMm2S)
	{
		xil_printf(MB_PROMPT "Device lacks MM2S channel\r\n");

		return XST_FAILURE;
	}
	if(!AxiDma->HasS2Mm)
	{
		xil_printf(MB_PROMPT "Device lacks S2MM channel\r\n");

		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

u32 XAxiDma_Halted(XAxiDma *InstancePtr, int Direction)
{
	return ((XAxiDma_ReadReg(InstancePtr->RegBase +
				(XAXIDMA_RX_OFFSET * Direction),
				XAXIDMA_SR_OFFSET) &
				XAXIDMA_HALTED_MASK) ? TRUE : FALSE);
}

// Basically Busy && !Halted, except with a single atomic AXI Read
u32 XAxiDma_Transceiving(XAxiDma *InstancePtr, int Direction)
{
	u32 status_reg=XAxiDma_ReadReg(InstancePtr->RegBase +
					(XAXIDMA_RX_OFFSET * Direction),
					XAXIDMA_SR_OFFSET);
	u8 running = (status_reg & XAXIDMA_HALTED_MASK)==0;
	u8 busy = (status_reg & XAXIDMA_IDLE_MASK)==0;
	return busy && running;
}
