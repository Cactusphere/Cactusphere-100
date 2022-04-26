/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Copyright (c) 2020 Atmark Techno, Inc.
 * 
 * MIT License
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "mt3620-baremetal.h"
#include "mt3620-intercore.h"
#include "mt3620-gpio.h"
#include "mt3620-timer.h"

#include "InterCoreComm.h"
#include "TimerUtil.h"
#include "UartDriveMsg.h"


const int TIMEOUT = 400; // 400[ms]

#define OK  1
#define NG  -1

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#define UART_CLOCK		(26000000UL)
#define UART_LCR				(0xc)
#define UART_DLL				(0x0)
#define UART_DLH				(0x4)
#define UART_RATE_STEP			(0x24)
#define UART_SAMPLE_COUNT		(0x28)
#define UART_SAMPLE_POINT		(0x2c)
#define UART_FRACDIV_L			(0x54)
#define UART_FRACDIV_M			(0x58)
#define UART_LCR_DLAB			(1 << 7)
#define UART_LCR_SB      		(1 << 6)
#define UART_LCR_SP 	    	(1 << 5)
#define UART_LCR_EPS_SHIFT		(4)
#define UART_LCR_PEN    		(1 << 3)
#define UART_LCR_STB_SHIFT		(2)
#define UART_LCR_WLS_SHIFT		(0)

#define RX_BUFFER_SIZE 10

extern uint32_t StackTop; // &StackTop == end of TCM

static _Noreturn void DefaultExceptionHandler(void);

// ISU3 UART Base Address
static const uintptr_t UART_BASE = 0x380a0500;


static
void Uart_Init(void)
{
    // Configure UART to use 115200-8-N-1.
    WriteReg32(UART_BASE, 0x0C, 0x80); // LCR (enable DLL, DLM)
    WriteReg32(UART_BASE, 0x24, 0x3);  // HIGHSPEED
    WriteReg32(UART_BASE, 0x04, 0);    // Divisor Latch (MS)
    WriteReg32(UART_BASE, 0x00, 1);    // Divisor Latch (LS)
    WriteReg32(UART_BASE, 0x28, 224);  // SAMPLE_COUNT
    WriteReg32(UART_BASE, 0x2C, 110);  // SAMPLE_POINT
    WriteReg32(UART_BASE, 0x58, 0);    // FRACDIV_M
    WriteReg32(UART_BASE, 0x54, 223);  // FRACDIV_L
    WriteReg32(UART_BASE, 0x0C, 0x03); // LCR (8-bit word length)
}

static
void mtk_hdl_uart_set_params(u32 baudrate, u8 parity, u8 stop)
{
    u8 uart_lcr, fraction, word_length;
    u32 data, high_speed_div, sample_count, sample_point;
    u8 fraction_L_mapping[] = { 0x00, 0x10, 0x44, 0x92, 0x59,
            0xab, 0xb7, 0xdf, 0xff, 0xff, 0xff };
    u8 fraction_M_mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x03 };

    /* Clear fraction */
    WriteReg32(UART_BASE, UART_FRACDIV_L, 0x00);
    WriteReg32(UART_BASE, UART_FRACDIV_M, 0x00);

    /* High speed mode */
    WriteReg32(UART_BASE, UART_RATE_STEP, 0x03);

    /* Set parity and stop bit */
    uart_lcr = ReadReg32(UART_BASE, UART_LCR);
    word_length = stop - 1;
    if(parity) {
        uart_lcr = uart_lcr | ((parity -1 ) << UART_LCR_EPS_SHIFT)
                    | UART_LCR_PEN;
        word_length += 1;
    }
    uart_lcr = uart_lcr | ((stop - 1) << UART_LCR_STB_SHIFT)
                | word_length << UART_LCR_WLS_SHIFT;
    WriteReg32(UART_BASE, UART_LCR, uart_lcr);

    /* DLAB start */
    uart_lcr = ReadReg32(UART_BASE, UART_LCR);
    WriteReg32(UART_BASE, UART_LCR, uart_lcr | UART_LCR_DLAB);

    data = UART_CLOCK / baudrate;
    /* divided by 256 */
    high_speed_div = (data >> 8) + 1;

    sample_count = data / high_speed_div - 1;
    /* threshold value */
    if (sample_count == 3)
        sample_point = 0;
    else
        sample_point = (sample_count + 1) / 2 - 2;

    /* check uart_clock, prevent calculation overflow */
    fraction = ((UART_CLOCK * 10 / baudrate * 10 / high_speed_div -
        (sample_count + 1) * 100) * 10 + 55) / 100;

    WriteReg32(UART_BASE, UART_DLL, (high_speed_div & 0x00ff));
    WriteReg32(UART_BASE, UART_DLH, (high_speed_div >> 8) & 0x00ff);
    WriteReg32(UART_BASE, UART_SAMPLE_COUNT, sample_count);
    WriteReg32(UART_BASE, UART_SAMPLE_POINT, sample_point);
    WriteReg32(UART_BASE, UART_FRACDIV_M, fraction_M_mapping[fraction]);
    WriteReg32(UART_BASE, UART_FRACDIV_L, fraction_L_mapping[fraction]);

    /* DLAB end */
    WriteReg32(UART_BASE, UART_LCR, uart_lcr);
}


static void
Uart_WritePoll(const char *msg, int len)
{
    // write 1 byte at a time while waiting for data register space,
    // then wait for transmission done (i.e. shift register becomes empty)
    //
    // LSR (offset 0x14)'s bit 5 and 6 are denote the status of
    // the data write register and the shift register respectively
    while (len--) {
        while (!(ReadReg32(UART_BASE, 0x14) & (UINT32_C(1) << 5))) {
            // just wait
        }

        WriteReg32(UART_BASE, 0x0, *msg++);
    }
    while (!(ReadReg32(UART_BASE, 0x14) & (UINT32_C(1) << 5))) {
        // just wait
    }

    while (!(ReadReg32(UART_BASE, 0x14) & (UINT32_C(1) << 6))) {
        // just wait
    }
}

static bool
Uart_ReadPoll(uint8_t *buffer, int len) {
    int val;
    int counter = 0;
    int recvTime = TimerUtil_GetTickCount();

    // read 1 byte at a time while waiting for data register fill
    //
    // LSR (offset 0x14)'s bit 0 denotes the status of the data read register
    memset(buffer, 0, len);
    for (int i = 0; i < len; i++) {
        while (0 == (ReadReg32(UART_BASE, 0x14) & 0x01)) {
            if (TimerUtil_GetTickCount() - recvTime > TIMEOUT) {
                return false;  // timed out
            }
        }
        val = (uint8_t)ReadReg32(UART_BASE, 0x00);
        if (val != 0) {
            buffer[counter++] = val;
        } else if (val == 0 && counter > 0) {
            buffer[counter++] = val;
        }
        recvTime = TimerUtil_GetTickCount();
    }

    return true;
}

static void
Uart_DataSkip() {
    uint8_t dummy;

    while (1 == (ReadReg32(UART_BASE, 0x14) & 0x01)) {
        dummy = (uint8_t)ReadReg32(UART_BASE, 0x00);
        (void)dummy;
    }
}

static _Noreturn void RTCoreMain(void);

// ARM DDI0403E.d SB1.5.2-3
// From SB1.5.3, "The Vector table must be naturally aligned to a power of two whose alignment
// value is greater than or equal to (Number of Exceptions supported x 4), with a minimum alignment
// of 128 bytes.". The array is aligned in linker.ld, using the dedicated section ".vector_table".

// The exception vector table contains a stack pointer, 15 exception handlers, and an entry for
// each interrupt.
#define INTERRUPT_COUNT 100 // from datasheet
#define EXCEPTION_COUNT (16 + INTERRUPT_COUNT)
#define INT_TO_EXC(i_) (16 + (i_))
const uintptr_t ExceptionVectorTable[EXCEPTION_COUNT] __attribute__((section(".vector_table")))
__attribute__((used)) = {
    [0] = (uintptr_t)&StackTop,                // Main Stack Pointer (MSP)
    [1] = (uintptr_t)RTCoreMain,               // Reset
    [2] = (uintptr_t)DefaultExceptionHandler,  // NMI
    [3] = (uintptr_t)DefaultExceptionHandler,  // HardFault
    [4] = (uintptr_t)DefaultExceptionHandler,  // MPU Fault
    [5] = (uintptr_t)DefaultExceptionHandler,  // Bus Fault
    [6] = (uintptr_t)DefaultExceptionHandler,  // Usage Fault
    [11] = (uintptr_t)DefaultExceptionHandler, // SVCall
    [12] = (uintptr_t)DefaultExceptionHandler, // Debug monitor
    [14] = (uintptr_t)DefaultExceptionHandler, // PendSV
    [15] = (uintptr_t)DefaultExceptionHandler, // SysTick

    [INT_TO_EXC(0)] = (uintptr_t)DefaultExceptionHandler,
    [INT_TO_EXC(1)] = (uintptr_t)Gpt_HandleIrq1,
    [INT_TO_EXC(2)... INT_TO_EXC(INTERRUPT_COUNT - 1)] = (uintptr_t)DefaultExceptionHandler };

static _Noreturn void
DefaultExceptionHandler(void)
{
    for (;;) {
        // empty.
    }
}

static _Noreturn void
RTCoreMain(void)
{
    uint8_t rxBuffer[RX_BUFFER_SIZE];
    bool initializeUart = false;

    // SCB->VTOR = ExceptionVectorTable
    WriteReg32(SCB_BASE, 0x08, (uint32_t)ExceptionVectorTable);

    // initialization
    if (! TimerUtil_Initialize()) {
        DefaultExceptionHandler();
    }
    if (! InterCoreComm_Initialize()) {
        DefaultExceptionHandler();
    }

    // for debugger connection (wait 3[sec])
    {
        uint32_t	prevTickCount = TimerUtil_GetTickCount();
        uint32_t	tickCount     = prevTickCount;


        while (3000 > tickCount - prevTickCount) {
            TimerUtil_SleepUntilIntr();
            tickCount = TimerUtil_GetTickCount();
        }
    }

    // GPIO setting
    static const GpioBlock grp5 = {
    .baseAddr = 0x38060000,.type = GpioBlock_GRP,.firstPin = 20,.pinCount = 4 };
    Mt3620_Gpio_AddBlock(&grp5);

    // setting up RS-485 receive mode
        // DE (disable)
    Mt3620_Gpio_ConfigurePinForOutput(21);
    Mt3620_Gpio_Write(21, false);
        // RE_N (enable)
    Mt3620_Gpio_ConfigurePinForOutput(23);
    Mt3620_Gpio_Write(23, false);

    // main loop
    for (;;) {
        // wait and receive a request message from HLApp and process it
        const UART_DriverMsg* msg = InterCoreComm_WaitAndRecvRequest();

        if (msg != NULL) {
            UART_ReturnMsg    retMsg;

            switch (msg->header.requestCode) {
            case UART_REQ_WRITE_AND_READ:
                if (initializeUart) {
                    Uart_DataSkip();  // read out unknown received data

                    // send request to the opposing device via RS-485
                    Mt3620_Gpio_Write(21, true);  // DE (enable)
                    Mt3620_Gpio_Write(23, true);  // RE_N (disable)
                    Uart_WritePoll((const char*)msg->body.writeAndReadReq.writeData,
                        msg->body.writeAndReadReq.writeLen);

                    // receive response from the opposing device
                    Mt3620_Gpio_Write(21, false);
                    Mt3620_Gpio_Write(23, false);

                    // send back the response to HLApp
                    if (! Uart_ReadPoll(rxBuffer, msg->body.writeAndReadReq.readLen)) {
                        memset(rxBuffer, 0, msg->body.writeAndReadReq.readLen);
                        if (InterCoreComm_SendReadData(rxBuffer, msg->body.writeAndReadReq.readLen)) {
 //                           int i = -1;
                        }
                    } else if (InterCoreComm_SendReadData(rxBuffer, msg->body.writeAndReadReq.readLen)) {
 //                       int i = 1;
                    }
                //
                // NOTE: It should make time for equal of transmitting 3.5 characters 
                //       according to Modbus RTU specification.
                //       (in case of 9600bps, about 3[ms])
                }
                break;
            case UART_REQ_SET_PARAMS:
                // initialize UART with requested params, then send back the status code
                // status code is
                //   0: error, 1: OK
                Uart_Init();
                mtk_hdl_uart_set_params(msg->body.setParams.baudRate,
                    msg->body.setParams.parity, msg->body.setParams.stop);
                initializeUart = true;
                if (! InterCoreComm_SendIntValue(1)) {
//                    int i = 0;
                }
                break;
            case UART_REQ_VERSION:
                memset(retMsg.message.version, 0x00, sizeof(retMsg.message.version));
                strncpy(retMsg.message.version, RTAPP_VERSION, strlen(RTAPP_VERSION) + 1);
                retMsg.returnCode = OK;
                retMsg.messageLen = strlen(RTAPP_VERSION);
                if (InterCoreComm_SendReadData((uint8_t*)&retMsg, sizeof(UART_ReturnMsg))) {
                    ;
                }
                break;
            default:
                break;
            }
        }
    }
}
