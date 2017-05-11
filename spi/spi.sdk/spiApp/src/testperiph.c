/******************************************************************************
*
* Copyright (C) 2002 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/******************************************************************************/
/**
* @file  xspi_low_level_example.c
*
* This file contains a design example using the low-level driver of the
* SPI driver (XSpi). These macros are found in xspi_l.h.  A simple loopback
* test is done within an SPI device in polled mode. This example works only with
* 8-bit wide data transfers.
*
* @note
* This example works only with 8-bit wide data transfers in standard SPI mode.
* This example will not work if the axi_qspi device is confiured in dual/quad
* modes.
*
* To make this example work for 16 bit transfers change u8 Buffer[BUFFER_SIZE]
* to u16 Buffer[BUFFER_SIZE]. The SPI Core should aslo be configured for 16 bit
* access during the build time.
*
* To make this example work for 32 bit transfers change u8 Buffer[BUFFER_SIZE]
* to u32 Buffer[BUFFER_SIZE]. The SPI Core should aslo be configured for 32 bit
* access during the build time.
*
*
*<pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- ----------------------------------------------------------
* 1.00b rpm  04/24/02 First release
* 1.00b jhl  09/10/02 Added code to ensure it works with a fast processor.
* 1.00b sv   05/16/05 Minor changes to comply to Doxygen and coding guidelines
* 3.00a ktn  10/28/09 Converted all register accesses to 32 bit access.
* 3.02a sdm  05/04/11 Added a note about dual/quad modes in axi_qspi.
*
*</pre>
*******************************************************************************/

/***************************** Include Files **********************************/
#include <stdio.h>
#include "xparameters.h"
#include "xstatus.h"
#include "xspi.h"
#include "spi_header.h"
#include "xil_cache.h"
/*************************** SPI Function Prototypes ***************************/
static inline void WaitForDataByte( void );
void WF_SpiInit(void);
static void ConfigureSpiMRF24W(void);
void WF_SpiEnableChipSelect(void);
void WF_SpiDisableChipSelect(void);

/*************************** SPI Constant/Macro definitions ********************************/
#define ClearSPIDoneFlag()  //{WF_SPI_IF = 0;}
#define SPI_ON_BIT          //(WF_SPICON1bits.SSPEN)
/************************** Constant Definitions ******************************/
/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define SPI_BASEADDR		XPAR_SPI_0_BASEADDR

/**************************** Type Definitions ********************************/


/***************** Macros (Inline Functions) Definitions **********************/


/************************** Function Prototypes *******************************/

int XSpi_LowLevelExample(u32 BaseAddress);

/************************** Variable Definitions ******************************/


/*
 *  This is the size of the buffer to be transmitted/received in this example.
 */
#define BUFFER_SIZE			 16

/*
 * The buffer used for Transmission/Reception of the SPI test data
 */
u16 Buffer[BUFFER_SIZE];

/******************************************************************************/
/**
* This function is the main function of the SPI Low Level example.
*
* @param	None
*
* @return	XST_SUCCESS to indicate success, else XST_FAILURE to indicate
*		Failure.
*
* @note		None
*
*******************************************************************************/
int main(void)
{
	int Status;
   Xil_ICacheEnable();
   Xil_DCacheEnable();
	/*
	 * Run the example, specify the Base Address that is generated in
	 * xparameters.h
	 */
	print("Testing SPI\r\n");
	Status = XSpi_LowLevelExample(SPI_BASEADDR);
	if (Status != XST_SUCCESS) {
		print("Failed\r\n");
		return XST_FAILURE;
	}
	print("Success\r\n");
	return XST_SUCCESS;
   Xil_DCacheDisable();
   Xil_ICacheDisable();
}

/******************************************************************************/
/**
*
* This function does a simple loopback test within an SPI device.
*
* @param	BaseAddress is the BaseAddress of the SPI device
*
* @return	XST_SUCCESS if successful, XST_FAILURE if unsuccessful
*
* @note		Nonet
*
*******************************************************************************/
int XSpi_LowLevelExample(u32 BaseAddress)
{
	u32 Control;
	u32 SlaveSelect;
	int NumBytesSent = 0;
	int NumBytesRcvd = 0;
	u32 Count;

	/*
	 * Set up the device in loopback mode and enable master mode.
	 */
	Control = XSpi_ReadReg(BaseAddress, XSP_CR_OFFSET);
	Control |= (XSP_CR_LOOPBACK_MASK | XSP_CR_MASTER_MODE_MASK);
	XSpi_WriteReg(BaseAddress, XSP_CR_OFFSET, Control);
	/************* SLAVE SELECT TOGGLE TEST ***************
	SlaveSelect = XSpi_ReadReg(BaseAddress,XSP_SSR_OFFSET);
	SlaveSelect &= ~0x01;
	XSpi_WriteReg(BaseAddress,XSP_SSR_OFFSET,SlaveSelect);
	*******************************************************/

	/*
	 * Initialize the buffer with some data.
	 */
	for (Count = 0; Count < BUFFER_SIZE; Count++) {
		Buffer[Count] = Count;
		//xil_printf("Write buff[%d] = %d\r\n",Count,Buffer[Count]);
	}

	/*
	 * Fill up the transmitter with data, assuming the receiver can hold
	 * the same amount of data.
	 */
	while ((XSpi_ReadReg(BaseAddress, XSP_SR_OFFSET) &
			XSP_SR_TX_FULL_MASK) == 0) {
		XSpi_WriteReg((BaseAddress), XSP_DTR_OFFSET,
				Buffer[NumBytesSent++]);
	}

	/*
	 * Enable the device.
	 */
	Control = XSpi_ReadReg(BaseAddress, XSP_CR_OFFSET);
	Control |= XSP_CR_ENABLE_MASK;
	Control &= ~XSP_CR_TRANS_INHIBIT_MASK;
	XSpi_WriteReg(BaseAddress, XSP_CR_OFFSET, Control);

	/*
	 * Initialize the buffer with zeroes so that it can be used to receive
	 * data.
	 */
	for (Count = 0; Count < BUFFER_SIZE; Count++) {
		Buffer[Count] = 0x0;
	}

	/*
	 * Wait for the transmit FIFO to transition to empty before checking
	 * the receive FIFO, this prevents a fast processor from seeing the
	 * receive FIFO as empty
	 */
	while (!(XSpi_ReadReg(BaseAddress, XSP_SR_OFFSET) &
					XSP_SR_TX_EMPTY_MASK));

	/*
	 * Transmitter is full, now receive the data just looped back until
	 * the receiver is empty.
	 */
	while ((XSpi_ReadReg(BaseAddress, XSP_SR_OFFSET) &
			XSP_SR_RX_EMPTY_MASK) == 0) {
		Buffer[NumBytesRcvd++] = XSpi_ReadReg((BaseAddress),
						XSP_DRR_OFFSET);
	}
	for(Count = 0;Count < BUFFER_SIZE; Count++){
		xil_printf("Read buff[%d] = %d\r\n",Count,Buffer[Count]);
	}
	/*
	 * If no data was sent or the data that was sent was not received,
	 * then return an error
	 */
	if ((NumBytesSent != NumBytesRcvd) || (NumBytesSent == 0)) {
		return XST_FAILURE;
	}
	return XST_SUCCESS;
}
/* Wifi_Spi.c Functions */
static inline void WaitForDataByte( void ){
	//while ((WF_SPISTATbits.SPITBF == 1) || (WF_SPISTATbits.SPIRBF == 0));
	//while (!WF_SPISTATbits.SPITBE || !WF_SPISTATbits.SPIRBF);
}
/*****************************************************************************
  Function:
	void WF_SpiInit(void)

  Summary:
	Initializes the SPI interface to the MRF24W device.

  Description:
	Configures the SPI interface for communications with the MRF24W.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	None

  Remarks:
	This function is called by WFHardwareInit.
*****************************************************************************/
void WF_SpiInit(void)
{
    /* disable the spi interrupt */
    #if defined( __PIC32MX__ )
        //WF_SPI_IE_CLEAR = WF_SPI_INT_BITS;
    #else
        //WF_SPI_IE = 0;
    #endif
    #if defined( __18CXX)
        //WF_SPI_IP = 0;
    #endif

    // Set up the SPI module on the PIC for communications with the MRF24W
    //WF_CS_IO       = 1;
    //WF_CS_TRIS     = 0;     // Drive SPI MRF24W chip select pin
    #if defined( __18CXX)
        //WF_SCK_TRIS    = 0;     /* SPI Clock is an output       */
        //WF_SDO_TRIS    = 0;     /* SPI Data Out is an output    */
        //WF_SDI_TRIS    = 1;     /* SPI Data In is an input      */
    #else
        // We'll let the module control the pins.
    #endif

    #if !defined( SPI_IS_SHARED )
    //ConfigureSpiMRF24W();
    #endif

    /* clear the completion flag */
    ClearSPIDoneFlag();
}
/*****************************************************************************
  Function:
	void ConfigureSpiMRF24W(void)

  Summary:
	Configures the SPI interface to the MRF24W.

  Description:
	Configures the SPI interface for communications with the MRF24W.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	None

  Remarks:
	1) If the SPI bus is shared with other peripherals this function is called
	each time an SPI transaction occurs by WF_SpiEnableChipSelect.  Otherwise it
	is called once during initialization by WF_SpiInit.

	2) Maximum SPI clock rate for the MRF24W is 25MHz.
*****************************************************************************/
static void ConfigureSpiMRF24W(void)
{
    /*----------------------------------------------------------------*/
    /* After we save context, configure SPI for MRF24W communications */
    /*----------------------------------------------------------------*/
    /* enable the SPI clocks            */
    /* set as master                    */
    /* clock idles high                 */
    /* ms bit first                     */
    /* 8 bit tranfer length             */
    /* data changes on falling edge     */
    /* data is sampled on rising edge   */
    /* set the clock divider            */
    #if defined(__18CXX)
        WF_SPICON1 = 0x20;      // SSPEN bit is set, SPI in master mode, (0x30 is for FOSC/4),
                                //   IDLE state is low level (0x32 is for FOSC/64)
        WF_SPISTATbits.CKE = 1; // Transmit data on falling edge of clock
        WF_SPISTATbits.SMP = 1; // Input sampled at end of data output time
    #elif defined(__C30__)
        WF_SPICON1 = 0x027B;    // Fcy Primary prescaler 1:1, secondary prescaler 2:1, CKP=1, CKE=0, SMP=1
        WF_SPICON2 = 0x0000;
        WF_SPISTAT = 0x8000;    // Enable the module
    #elif defined( __PIC32MX__ )

        #if defined(__Digilent_Build__)
            WF_SPI_BRG = ((GetPeripheralClock()/2ul/WF_MAX_SPI_FREQ)-1ul);
        #else
            WF_SPI_BRG = (GetPeripheralClock()-1ul)/2ul/WF_MAX_SPI_FREQ;
        #endif

        WF_SPICON1 = 0x00000260;    // sample at end, data change idle to active, clock idle high, master
        WF_SPICON1bits.ON = 1;
    #else
        //#error Configure SPI for the selected processor
    #endif
}
/*****************************************************************************
  Function:
	void WF_SpiEnableChipSelect(void)

  Summary:
	Enables the MRF24W SPI chip select.

  Description:
	Enables the MRF24W SPI chip select as part of the sequence of SPI
	communications.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	None

  Remarks:
	If the SPI bus is shared with other peripherals then the current SPI context
	is saved.
*****************************************************************************/
void WF_SpiEnableChipSelect(void)
{
    #if defined(__18CXX)
    static volatile UINT8 dummy;
    #endif

    #if defined(SPI_IS_SHARED)
    SaveSpiContext();
    ConfigureSpiMRF24W();
    #endif

    /* set Slave Select low (enable SPI chip select on MRF24W) */
    //WF_CS_IO = 0;

    /* clear any pending interrupts */
    #if defined(__18CXX)
        dummy = WF_SSPBUF;
        ClearSPIDoneFlag();
    #endif


}
/*****************************************************************************
  Function:
	void WF_SpiDisableChipSelect(void)

  Summary:
	Disables the MRF24W SPI chip select.

  Description:
	Disables the MRF24W SPI chip select as part of the sequence of SPI
	communications.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	None

  Remarks:
	If the SPI bus is shared with other peripherals then the current SPI context
	is restored.
*****************************************************************************/
void WF_SpiDisableChipSelect(void)
{
    /* Disable the interrupt */
    #if defined( __PIC32MX__ )
        WF_SPI_IE_CLEAR = WF_SPI_INT_BITS;
    #else
       // WF_SPI_IE = 0;
    #endif

    /* set Slave Select high ((disable SPI chip select on MRF24W)   */
    //WF_CS_IO = 1;

    #if defined(SPI_IS_SHARED)
    RestoreSpiContext();
    #endif
}
/*****************************************************************************
  Function:
	void WFSpiTxRx(void)

  Summary:
	Transmits and receives SPI bytes

  Description:
	Transmits and receives N bytes of SPI data.

  Precondition:
	None

  Parameters:
	p_txBuf - pointer to SPI tx data
	txLen   - number of bytes to Tx
	p_rxBuf - pointer to where SPI rx data will be stored
	rxLen   - number of SPI rx bytes caller wants copied to p_rxBuf

  Returns:
  	None

  Remarks:
	Will clock out the larger of txLen or rxLen, and pad if necessary.
*****************************************************************************/
void WFSpiTxRx(u8   *p_txBuf,
               u16  txLen,
               u8   *p_rxBuf,
               u16  rxLen)
{
    #if defined(__18CXX)
        static UINT16 byteCount;  /* avoid local variables in functions called from interrupt routine */
        static UINT16 i;
        static UINT8  rxTrash;
    #else
        u16 byteCount;
        u16 i;
        u8  rxTrash;
    #endif


#if defined(WF_DEBUG) && defined(WF_USE_POWER_SAVE_FUNCTIONS)
    /* Cannot communicate with MRF24W when it is in hibernate mode */
    {
        static UINT8 state;  /* avoid local vars in functions called from interrupt */
        WF_GetPowerSaveState(&state);
        WF_ASSERT(state != WF_PS_HIBERNATE);
    }
#endif

    /* total number of byte to clock is whichever is larger, txLen or rxLen */
    byteCount = (txLen >= rxLen)?txLen:rxLen;

    for (i = 0; i < byteCount; ++i)
    {
        /* if still have bytes to transmit from tx buffer */
        if (txLen > 0)
        {
           // WF_SSPBUF = *p_txBuf++;
            --txLen;
        }
        /* else done writing bytes out from tx buffer */
        else
        {
            //WF_SSPBUF = 0xff;  /* clock out a "don't care" byte */
        }

        /* wait until tx/rx byte to completely clock out */
        WaitForDataByte();

        /* if still have bytes to read into rx buffer */
        if (rxLen > 0)
        {
           // *p_rxBuf++ = WF_SSPBUF;
            //--rxLen;
        }
        /* else done reading bytes into rx buffer */
        else
        {
            //rxTrash = WF_SSPBUF;  /* read and throw away byte */
        }
    }  /* end for loop */

}
