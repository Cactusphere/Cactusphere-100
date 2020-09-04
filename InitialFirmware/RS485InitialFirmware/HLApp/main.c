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

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/storage.h>
#include <applibs/i2c.h>

#include <hw/mt3620.h>

#include "eventloop_timer_utilities.h"

#include "drivers/at24c0.h"
#include "ModbusDev.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They they must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,
    ExitCode_TermHandler_SigTerm,
    ExitCode_Main_EventLoopFail,
    ExitCode_LedTimer_Consume,
    ExitCode_Init_LedTimer,
    ExitCode_ScanTimer_Consume,
    ExitCode_Init_ScanTimer,
    ExitCode_SetUpSysEvent_EventLoop,
} ExitCode;

static volatile sig_atomic_t exitCode = ExitCode_Success;

#include "SendRTApp.h"

#include "cactusphere_product.h"
#include "cactusphere_eeprom.h"

// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// Status LED
enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK,
};
int gLedState = LED_OFF;
int ledGpioFd = -1;

// Status Scan
int slaveAddr = 1;
int regAddr = 0;

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *ledEventLoopTimer = NULL;
static EventLoopTimer *scanEventLoopTimer = NULL;

static void LedEventHandler(EventLoopTimer *timer);
static void ScanEventHandler(EventLoopTimer *timer);

typedef struct
{
    const char name[20];
    char value[20];
} EventMsgData;

typedef struct {
    const char name[20];
    char value[20];
} EepromPropertyData;

eepromData eeprom;
/// <summary>
///     Convert and Print EEPROM Data.
/// </summary>
static void convertEepromData(void)
{
    static EepromPropertyData getEepromData[] = {
        {"SerialNumber", ""},
        {"LanMacAddr", ""},
        {"ProductId", ""},
        {"VendorId", ""},
        {"WifiMacAddr", ""},
        {"Generation", ""},
    };

    setEepromString(getEepromData[0].value, eeprom.serial, sizeof(eeprom.serial));
    setEepromString(getEepromData[1].value, eeprom.ethernetMac, sizeof(eeprom.ethernetMac));
    setEepromString(getEepromData[2].value, eeprom.productId, sizeof(eeprom.productId));
    setEepromString(getEepromData[3].value, eeprom.venderId, sizeof(eeprom.venderId));
    setEepromString(getEepromData[4].value, eeprom.wlanMac, sizeof(eeprom.wlanMac));
    setEepromString(getEepromData[5].value, eeprom.generation, sizeof(eeprom.generation));

    for (int i = 0; i < (sizeof(getEepromData) / sizeof(getEepromData[0])); i++) {
        Log_Debug("%s : %s\n", getEepromData[i].name, getEepromData[i].value);
    }
}

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Starting Cactusphere RS485Model Initial Firmware %s\n", APP_VERSION);

    int retEeprom = GetEepromProperty(&eeprom);
    if (retEeprom < 0) {
        Log_Debug("ERROR: Get EEPROM Data.\n");
    } else {
        convertEepromData();
    }

    SendRTApp_InitHandlers();

    exitCode = InitPeripheralsAndHandlers();

    // Main loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);

        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    SendRTApp_CloseHandlers();
    ClosePeripheralsAndHandlers();

    Log_Debug("Application exiting.\n");

    return exitCode;
}

/// <summary>
/// LED event:  Opration LED
/// </summary>
void LedEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_LedTimer_Consume;
        return;
    }

    GPIO_Value_Type LED_State;
    switch (gLedState)
    {
    case LED_OFF:
        GPIO_SetValue(ledGpioFd, GPIO_Value_High);
        break;
    case LED_ON:
        GPIO_SetValue(ledGpioFd, GPIO_Value_Low);
        break;
    case LED_BLINK:
        GPIO_GetValue(ledGpioFd, &LED_State);
        GPIO_SetValue(ledGpioFd, (GPIO_Value_Type)(1 - LED_State));
        break;
    default:
        break;
    }
}

/// <summary>
/// Scan event: Scan RS485 slave
/// </summary>
void ScanEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ScanTimer_Consume;
        return;
    }
    ModbusDev *dev;
    unsigned short value;
    if (regAddr > 0x3) {
        regAddr = 0;
        slaveAddr++;
        if (slaveAddr > 0xff) {
            slaveAddr = 1;
        }
    }
    dev = ModbusDev_NewModbusRTU(slaveAddr, 9600);
    if(ModbusDev_Connect(dev)) {
        if(ModbusDev_ReadSingleRegister(dev, regAddr, &value)) {
            Log_Debug("id = %d, reg = %d, value = %d", slaveAddr, regAddr, value);
            gLedState = LED_BLINK;
        }
        if(ModbusDev_ReadSingleInputRegister(dev, regAddr, &value)) {
            Log_Debug("id = %d, reg = %d, value = %d", slaveAddr, regAddr, value);
            gLedState = LED_BLINK;
        }
    }
    regAddr++;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>ExitCode_Success if all resources were allocated successfully; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("Could not create event loop.\n");
        return ExitCode_SetUpSysEvent_EventLoop;
    }

    // LED
	Log_Debug("Opening MT3620_GPIO8 as output\n");
	ledGpioFd =
		GPIO_OpenAsOutput(MT3620_GPIO8, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (ledGpioFd < 0) {
		Log_Debug("ERROR: Could not open LED: %s (%d).\n", strerror(errno), errno);
		return ExitCode_TermHandler_SigTerm;
	}

    struct timespec checkLedStatusPeriod = {.tv_sec = 0, .tv_nsec = 500 * 1000 * 1000};
    ledEventLoopTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &LedEventHandler, &checkLedStatusPeriod);
    if (ledEventLoopTimer == NULL) {
        return ExitCode_Init_LedTimer;
    }

    struct timespec checkScanStatusPeriod = {.tv_sec = 0, .tv_nsec = 1000};
    scanEventLoopTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &ScanEventHandler, &checkScanStatusPeriod);
    if (scanEventLoopTimer == NULL) {
        return ExitCode_Init_ScanTimer;
    }

    // LED ON
    gLedState = LED_ON;

    return ExitCode_Success;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors\n");

    DisposeEventLoopTimer(ledEventLoopTimer);
    EventLoop_Close(eventLoop);
}
