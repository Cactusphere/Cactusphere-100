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
#include <applibs/networking.h>
#include <applibs/gpio.h>
#include <applibs/storage.h>
#include <applibs/i2c.h>

#include <hw/mt3620.h>

#include "eventloop_timer_utilities.h"

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>

#include "drivers/at24c0.h"

/// <summary>
/// Exit codes for this application. These are used for the
/// application exit code.  They they must all be between zero and 255,
/// where zero is reserved for successful termination.
/// </summary>
typedef enum {
    ExitCode_Success = 0,

    ExitCode_TermHandler_SigTerm = 1,

    ExitCode_Main_EventLoopFail = 2,

    ExitCode_AzureTimer_Consume = 3,
    ExitCode_WatchDogTimer_Consume = 4,
    ExitCode_LedTimer_Consume = 5,

    ExitCode_Init_EventLoop = 6,
    ExitCode_Init_AzureTimer = 7,
    ExitCode_Init_WatchDogTimer = 8,
    ExitCode_Init_LedTimer = 9,

    ExitCode_SetUpSysEvent_EventLoop = 10,
} ExitCode;

static volatile sig_atomic_t exitCode = ExitCode_Success;

#include "json.h"

#include "LibCloud.h"
#include "DataFetchScheduler.h"
#include "SendRTApp.h"
#include "TelemetryItems.h"
#include "PropertyItems.h"

#include "cactusphere_product.h"
#include "cactusphere_eeprom.h"
#include "cactusphere_error.h"

#define APP_VENDOR_ID   VENDER_ATMARK_TECHNO
#define APP_GENERATION  (1)

#if (APP_PRODUCT_ID == PRODUCT_ATMARK_TECHNO_DIN)
#define USE_DI
#define DI_PORT_OFFSET 1
#endif // PRODUCT_ATMARK_TECHNO_DIN

#if (APP_PRODUCT_ID == PRODUCT_ATMARK_TECHNO_RS485)
#define USE_MODBUS
#define USE_MODBUS_TCP
#endif // PRODUCT_ATMARK_TECHNO_RS485

#ifdef USE_MODBUS
#include "ModbusConfigMgr.h"
#include "ModbusFetchConfig.h"
#include "LibModbus.h"
#endif  // USE_MODBUS

#ifdef USE_MODBUS_TCP
#include "ModbusTcpConfigMgr.h"
#include "ModbusTcpFetchConfig.h"
#endif // USE_MODBUS_TCP

#ifdef USE_DI
#include "DI_ConfigMgr.h"
#include "DI_DataFetchScheduler.h"
#include "DI_FetchConfig.h"
#include "DI_WatchConfig.h"
#include "LibDI.h"
#endif  // USE_DI

static int ct_error = 0;

// cache buffer size (telemetry data)
#define CACHE_BUF_SIZE (50 * 1024)

// Azure IoT Hub/Central defines.
#define SCOPEID_LENGTH 20
static char scopeId[SCOPEID_LENGTH]; // ScopeId for the Azure IoT Central application, set in
                                     // app_manifest.json, CmdArgs

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int keepalivePeriodSeconds = 20;
static bool iothubAuthenticated = false;

static int CommandCallback(const char* method_name, const unsigned char* payload, size_t size,
    unsigned char** response, size_t* response_size, void* userContextCallback);
static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                         size_t payloadSize, void *userContextCallback);
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason);
static const char *getAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult);
static void SetupAzureClient(void);

// Initialization/Cleanup
static ExitCode InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// Software Watchdog
const struct itimerspec watchdogInterval = { { 300, 0 },{ 300, 0 } };
timer_t watchdogTimer;

// Status LED
enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK,
};
int gLedState = LED_OFF;
int ledGpioFd = -1;

// Timer / polling
static EventLoop *eventLoop = NULL;
static EventLoopTimer *azureTimer = NULL;
static EventLoopTimer *watchdogLoopTimer = NULL;
static EventLoopTimer *ledEventLoopTimer = NULL;

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 1; // 1[s]
static const int AzureIoTMinReconnectPeriodSeconds = 60;
static const int AzureIoTMaxReconnectPeriodSeconds = 10 * 60;

static int azureIoTPollPeriodSeconds = -1;

#define MAX_SCHEDULER_NUM   3
static DataFetchScheduler* mTelemetrySchedulerArr[MAX_SCHEDULER_NUM] = { NULL };

static void AzureTimerEventHandler(EventLoopTimer *timer);
static void WatchdogEventHandler(EventLoopTimer *timer);
static void LedEventHandler(EventLoopTimer *timer);

typedef struct
{
    const char name[20];
    char value[20];
} EventMsgData;

eepromData eeprom;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    exitCode = ExitCode_TermHandler_SigTerm;
}

bool
IsAuthenticationDone(void)
{
    return iothubAuthenticated;
}

void SetupWatchdog(void)
{
    struct sigevent alarmEvent;
    alarmEvent.sigev_notify = SIGEV_SIGNAL;
    alarmEvent.sigev_signo = SIGALRM;
    alarmEvent.sigev_value.sival_ptr = &watchdogTimer;

    timer_create(CLOCK_MONOTONIC, &alarmEvent, &watchdogTimer);
    timer_settime(watchdogTimer, 0, &watchdogInterval, NULL);
}

// Must be called periodically
void ExtendWatchdogExpiry(void)
{
    //check that application is operating normally
    //if so, reset the watchdog
    timer_settime(watchdogTimer, 0, &watchdogInterval, NULL);
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    int err;
    Log_Debug("IoT Hub/Central Application starting.\n");

#ifdef USE_MODBUS
    mTelemetrySchedulerArr[MODBUS_RTU] = Factory_CreateScheduler(MODBUS_RTU);
    ModbusConfigMgr_Initialize();
#endif  // USE_MODBUS
#ifdef USE_MODBUS_TCP
    mTelemetrySchedulerArr[MODBUS_TCP] = Factory_CreateScheduler(MODBUS_TCP);
    ModbusTcpConfigMgr_Initialize();
#endif  // USE_MODBUS_TCP
#ifdef USE_DI
    mTelemetrySchedulerArr[DIGITAL_IN] = Factory_CreateScheduler(DIGITAL_IN);
    DI_ConfigMgr_Initialize();
#endif  // USE_DI

    TelemetryItems_InitDictionary();
    SendRTApp_InitHandlers();

    if (argc == 2) {
        Log_Debug("Setting Azure Scope ID %s\n", argv[1]);
        strncpy(scopeId, argv[1], SCOPEID_LENGTH);
    } else {
        Log_Debug("ScopeId needs to be set in the app_manifest CmdArgs\n");
        return -1;
    }
    err = Networking_SetInterfaceState("eth0", true);
    if (err < 0) {
        Log_Debug("Error setting interface state (eth0) %d\n", errno);
    }

    exitCode = InitPeripheralsAndHandlers();

    // Main loop
    while (exitCode == ExitCode_Success) {
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }

    TelemetryItems_CleanupDictionary();
#ifdef USE_MODBUS
    ModbusConfigMgr_Cleanup();
#endif  // USE_MODBUS

#ifdef USE_MODBUS_TCP
    ModbusTcpConfigMgr_Cleanup();
#endif  // USE_MODBUS_TCP

#ifdef USE_DI
    DI_ConfigMgr_Cleanup();
#endif  // USE_DI

    for (int i = 0; i < MAX_SCHEDULER_NUM; i++) {
        DataFetchScheduler* scheduler = mTelemetrySchedulerArr[i];
        if (NULL != scheduler) {
            DataFetchScheduler_Destroy(scheduler);
        }
    }

    SendRTApp_CloseHandlers();

    while(ct_error < 0) {
        // hang
        EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
        // Continue if interrupted by signal, e.g. due to breakpoint being set.
        if (result == EventLoop_Run_Failed && errno != EINTR) {
            exitCode = ExitCode_Main_EventLoopFail;
        }
    }
    ClosePeripheralsAndHandlers();

    Log_Debug("Application exiting.\n");

    return exitCode;
}

/// <summary>
/// Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_AzureTimer_Consume;
        return;
    }

    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (! isNetworkReady) {
            iothubAuthenticated = false;
            gLedState = LED_BLINK;
        } else if (! iothubAuthenticated) {
            SetupAzureClient();
            IoT_CentralLib_Initialize(
                CACHE_BUF_SIZE, false);
        }
    } else {
        Log_Debug("Failed to get Network state\n");
    }

    if (ct_error < 0) {
        goto dowork;
    }

    for (int i = 0; i < MAX_SCHEDULER_NUM; i++) {
        DataFetchScheduler* scheduler = mTelemetrySchedulerArr[i];

        if (NULL != scheduler) {
            DataFetchScheduler_Schedule(scheduler);
        }
    }
    
dowork:
    if (iothubAuthenticated) {
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

static void WatchdogEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_WatchDogTimer_Consume;
        return;
    }

    ExtendWatchdogExpiry();
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

    SetupWatchdog();
    struct timespec watchdogKickPeriod = {.tv_sec = 0, .tv_nsec = 500 * 1000 * 1000};
    watchdogLoopTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &WatchdogEventHandler, &watchdogKickPeriod);
    if (watchdogLoopTimer == NULL) {
        return ExitCode_Init_WatchDogTimer;
    }

    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    azureTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
    if (azureTimer == NULL) {
        return ExitCode_Init_AzureTimer;
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

    DisposeEventLoopTimer(azureTimer);
    DisposeEventLoopTimer(watchdogLoopTimer);
    DisposeEventLoopTimer(ledEventLoopTimer);
    EventLoop_Close(eventLoop);
}

/// <summary>
///     Sets the IoT Hub authentication state for the app
///     The SAS Token expires which will set the authentication state
/// </summary>
static void HubConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result,
                                        IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason,
                                        void *userContextCallback)
{
    iothubAuthenticated = (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED);
    Log_Debug("IoT Hub Authenticated: %s\n", GetReasonString(reason));

    if (reason == IOTHUB_CLIENT_CONNECTION_OK) {

        int ret;
        Log_Debug("Getting EEPROM information.\n");
        ret = GetEepromProperty(&eeprom);
        if (ret < 0) {
            ct_error = -EEPROM_READ_ERROR;
            gLedState = LED_BLINK;
            cactusphere_error_notify(EEPROM_READ_ERROR);
            exitCode = ExitCode_TermHandler_SigTerm;
        } else {
            static const char* EventMsgTemplate = "{ \"%s\": \"%s\" }";
            static char propertyStr[280] = { 0 };

            // Send App version
            // HLApp
            snprintf(propertyStr, sizeof(propertyStr), EventMsgTemplate, "HLAppVersion", HLAPP_VERSION);
            IoT_CentralLib_SendProperty(propertyStr);
            // RTApp
            char rtAppVersion[256] = { 0 };
            bool ret = false;
#if defined USE_DI
            ret = DI_Lib_ReadRTAppVersion(rtAppVersion);
#elif defined USE_MODBUS
            ret = Libmodbus_GetRTAppVersion(rtAppVersion);
#endif
            if (ret) {
                snprintf(propertyStr, sizeof(propertyStr), EventMsgTemplate, "RTAppVersion", rtAppVersion);
                IoT_CentralLib_SendProperty(propertyStr);
            }

            gLedState = LED_ON;

            static EventMsgData eepromProperty[] = {
                {"SerialNumber", ""},
                {"EthMacAddr", ""},
                {"ProductId", ""},
                {"VendorId", ""},
                {"WlanMacAddr", ""},
                {"Generation", ""},
            };

            setEepromString(eepromProperty[0].value, eeprom.serial, sizeof(eeprom.serial));
            setEepromString(eepromProperty[1].value, eeprom.ethernetMac, sizeof(eeprom.ethernetMac));
            setEepromString(eepromProperty[2].value, eeprom.productId, sizeof(eeprom.productId));
            setEepromString(eepromProperty[3].value, eeprom.venderId, sizeof(eeprom.venderId));
            setEepromString(eepromProperty[4].value, eeprom.wlanMac, sizeof(eeprom.wlanMac));
            setEepromString(eepromProperty[5].value, eeprom.generation, sizeof(eeprom.generation));

            // Send Property
            for (int i = 0; i < (sizeof(eepromProperty) / sizeof(eepromProperty[0])); i++) {
                snprintf(propertyStr, sizeof(propertyStr), EventMsgTemplate, eepromProperty[i].name, eepromProperty[i].value);
                IoT_CentralLib_SendProperty(propertyStr);
            }
            if ((eeprom.venderId[0] | ((eeprom.venderId[1] << 8) & 0xFF00)) != APP_VENDOR_ID) {
                Log_Debug("ERROR: Illegal vendor id.\n");
                ct_error = -ILLEGAL_PACKAGE;
            }
            if ((eeprom.productId[0] | ((eeprom.productId[1] << 8) & 0xFF00)) != APP_PRODUCT_ID) {
                Log_Debug("ERROR: Illegal product id.\n");
                ct_error = -ILLEGAL_PACKAGE;
            }
            if (eeprom.generation[0] != APP_GENERATION) {
                Log_Debug("ERROR: Illegal generation.\n");
                ct_error = -ILLEGAL_PACKAGE;
            }
            if (ct_error < 0) {
                gLedState = LED_BLINK;
                cactusphere_error_notify(ILLEGAL_PACKAGE);
                exitCode = ExitCode_TermHandler_SigTerm;
            }
        }
    } else {
        gLedState = LED_BLINK;
    }
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetupAzureClient(void)
{
    if (iothubClientHandle != NULL)
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);

    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
                                                                          &iothubClientHandle);
    Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
              getAzureSphereProvisioningResultString(provResult));

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {
        gLedState = LED_BLINK;

        // If we fail to connect, reduce the polling frequency, starting at
        // AzureIoTMinReconnectPeriodSeconds and with a backoff up to
        // AzureIoTMaxReconnectPeriodSeconds
        if (azureIoTPollPeriodSeconds == AzureIoTDefaultPollPeriodSeconds) {
            azureIoTPollPeriodSeconds = AzureIoTMinReconnectPeriodSeconds;
        } else {
            azureIoTPollPeriodSeconds *= 2;
            if (azureIoTPollPeriodSeconds > AzureIoTMaxReconnectPeriodSeconds) {
                azureIoTPollPeriodSeconds = AzureIoTMaxReconnectPeriodSeconds;
            }
        }

        struct timespec azureTelemetryPeriod = {azureIoTPollPeriodSeconds, 0};
        SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

        Log_Debug("ERROR: failure to create IoTHub Handle - will retry in %i seconds.\n",
                  azureIoTPollPeriodSeconds);
        return;
    }

    // Successfully connected, so make sure the polling frequency is back to the default
    azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0};
    SetEventLoopTimerPeriod(azureTimer, &azureTelemetryPeriod);

    iothubAuthenticated = true;
    gLedState = LED_ON;

    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_KEEP_ALIVE,
                                        &keepalivePeriodSeconds) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure setting option \"%s\"\n", OPTION_KEEP_ALIVE);
        return;
    }

    IoTHubDeviceClient_LL_SetDeviceTwinCallback(iothubClientHandle, TwinCallback, NULL);
    IoTHubDeviceClient_LL_SetConnectionStatusCallback(iothubClientHandle,
                                                      HubConnectionStatusCallback, NULL);
    IoTHubDeviceClient_LL_SetDeviceMethodCallback(iothubClientHandle, CommandCallback, NULL);
}

/// <summary>
///    Send property response.
/// </summary>
#define JSON_FORMAT_NUM 50
static void SendPropertyResponse(vector Send_PropertyItem)
{
    static const char* EventMsgTemplate_bool = "{ \"%s\": %s }";
    static const char* EventMsgTemplate_num  = "{ \"%s\": %u }";
    static const char* EventMsgTemplate_str  = "{ \"%s\": \"%s\" }";
    char* propertyStr = NULL;
    size_t propertyStr_len = 0;
    
    if (! vector_is_empty(Send_PropertyItem)) {
        ResponsePropertyItem* curs = (ResponsePropertyItem*)vector_get_data(Send_PropertyItem);
        for (int i = 0; i < vector_size(Send_PropertyItem); i++){
            switch (curs->type)
            {
            case TYPE_BOOL:
                propertyStr_len = strlen(curs->propertyName) + JSON_FORMAT_NUM;
                propertyStr = (char *)malloc(propertyStr_len);
                if (propertyStr) {
                    memset(propertyStr, 0, propertyStr_len);
                    snprintf(propertyStr, propertyStr_len, EventMsgTemplate_bool,
                             curs->propertyName, (curs->value.b ? "true" : "false"));
                }
                break;
            case TYPE_NUM:
                propertyStr_len = strlen(curs->propertyName) + JSON_FORMAT_NUM;
                propertyStr = (char *)malloc(propertyStr_len);
                if (propertyStr) {
                    memset(propertyStr, 0, propertyStr_len);
                    snprintf(propertyStr, propertyStr_len, EventMsgTemplate_num,
                             curs->propertyName, curs->value.ul);
                }
                break;
            case TYPE_STR:
                propertyStr_len = strlen(curs->propertyName) + strlen(curs->value.str) + JSON_FORMAT_NUM;
                propertyStr = (char *)malloc(propertyStr_len);
                if(propertyStr) {
                    memset(propertyStr, 0, propertyStr_len);
                    snprintf(propertyStr, strlen(curs->propertyName)+strlen(curs->value.str) + JSON_FORMAT_NUM, EventMsgTemplate_str,
                             curs->propertyName, curs->value.str);
                }
                free(curs->value.str);
                break;
            default:
                continue;
            }
            if (propertyStr) {
                IoT_CentralLib_SendProperty(propertyStr);
                free(propertyStr);
                propertyStr = NULL;
            }
            curs++;
        }
    }
}

/// <summary>
///     Callback invoked when a Device Twin update is received from IoT Hub.
///     Updates local state for 'showEvents' (bool).
/// </summary>
/// <param name="payload">contains the Device Twin JSON document (desired and reported)</param>
/// <param name="payloadSize">size of the Device Twin JSON document</param>
static void TwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payload,
                         size_t payloadSize, void *userContextCallback)
{   
    if (ct_error < 0) {
        return;
    }

    vector Send_PropertyItem = vector_init(sizeof(ResponsePropertyItem));

#ifdef USE_MODBUS
    SphereWarning err = ModbusConfigMgr_LoadAndApplyIfChanged(payload, payloadSize, Send_PropertyItem);
    switch (err)
    {
    case NO_ERROR:
    case ILLEGAL_PROPERTY:
        DataFetchScheduler_Init(
            mTelemetrySchedulerArr[MODBUS_RTU],
            ModbusFetchConfig_GetFetchItemPtrs(ModbusConfigMgr_GetModbusFetchConfig()));
        
        if (err == NO_ERROR) {
            gLedState = LED_ON;
        } else { // ILLEGAL_PROPERTY
            // do not set ct_error and exitCode,
            // as it won't hang with this error.
            Log_Debug("ERROR: Receive illegal property.\n");
            gLedState = LED_BLINK;
            cactusphere_error_notify(err);
        }
        break;
    case ILLEGAL_DESIRED_PROPERTY:
        Log_Debug("ERROR: Receive illegal desired property.\n");
        ct_error = -(int)err;
        break;
    case UNSUPPORTED_PROPERTY:
        Log_Debug("ERROR: Receive unsupported property.\n");
        ct_error = -(int)err;
        break;

    default:
        break;
    }
    SendPropertyResponse(Send_PropertyItem);
#endif  // USE_MODBUS

#ifdef USE_MODBUS_TCP
    ModbusTcpConfigMgr_LoadAndApplyIfChanged(payload, payloadSize);
    DataFetchScheduler_Init(
        mTelemetrySchedulerArr[MODBUS_TCP],
        ModbusTcpFetchConfig_GetFetchItemPtrs(ModbusTcpConfigMgr_GetModbusFetchConfig()));
#endif // USE_MODBUS_TCP

#ifdef USE_DI
    SphereWarning err = DI_ConfigMgr_LoadAndApplyIfChanged(payload, payloadSize, Send_PropertyItem);
    switch (err)
    {
    case NO_ERROR:
    case ILLEGAL_PROPERTY:
        DI_DataFetchScheduler_Init(
            mTelemetrySchedulerArr[DIGITAL_IN],
            DI_FetchConfig_GetFetchItemPtrs(DI_ConfigMgr_GetFetchConfig()),
            DI_WatchConfig_GetFetchItems(DI_ConfigMgr_GetWatchConfig()));
        SendPropertyResponse(Send_PropertyItem);

        if (err == NO_ERROR) {
            gLedState = LED_ON;
        } else { // ILLEGAL_PROPERTY
            // do not set ct_error and exitCode,
            // as it won't hang with this error.
            Log_Debug("ERROR: Receive illegal property.\n");
            gLedState = LED_BLINK;
            cactusphere_error_notify(err);
        }
        break;
    case ILLEGAL_DESIRED_PROPERTY:
        Log_Debug("ERROR: Receive illegal desired property.\n");
        ct_error = -(int)err;
        break;
    case UNSUPPORTED_PROPERTY:
        Log_Debug("ERROR: Receive unsupported property.\n");
        ct_error = -(int)err;
        break;

    default:
        break;
    }

#endif  // USE_DI
    vector_destroy(Send_PropertyItem);

    if (ct_error < 0) {
        // hang
        gLedState = LED_BLINK;
        cactusphere_error_notify(err);
        exitCode = ExitCode_TermHandler_SigTerm;
    }
}

static int CommandCallback(const char* method_name, const unsigned char* payload, size_t size,
    unsigned char** response, size_t* response_size, void* userContextCallback) {
    
    if (ct_error < 0) {
        goto end;
    }

#ifdef USE_DI
    char deviceMethodResponse[100];
    char reportedPropertiesString[100];

    const char ClearCounterDIKey[] = "ClearCounter_DI";
    const size_t ClearCounterDiLen = strlen(ClearCounterDIKey);
    static const char* ReportMsgTemplate = "{ \"ClearCounterResult_DI%d\": \"%s\" }";

    if (0 == strncmp(method_name, ClearCounterDIKey, ClearCounterDiLen)) {
        int pinId = strtol(&method_name[ClearCounterDiLen], NULL, 10) - DI_PORT_OFFSET;
        if (pinId < 0) {
            goto err;
        }
        
        if (0 != strncmp(payload, "null", strlen("null"))) {
            char *cmdPayload = (char *)calloc(size + 1, sizeof(char));
            if (NULL == cmdPayload) {
                goto err;
            }
            strncpy(cmdPayload, payload, size);
            uint64_t initVal = strtoull(cmdPayload, NULL, 10);
            
            if (initVal > 0x7FFFFFFF) {
                goto err_value;
            }
            if (!DI_Lib_ResetPulseCount((unsigned long)pinId, (unsigned long)initVal)) {
                Log_Debug("DI_Lib_ResetPulseCount() error");
                strcpy(deviceMethodResponse, "\"Reset Error\"");
                snprintf(reportedPropertiesString, sizeof(reportedPropertiesString), ReportMsgTemplate, pinId + DI_PORT_OFFSET, "Reset Error");
            } else {
                strcpy(deviceMethodResponse, "\"Success\"");
                snprintf(reportedPropertiesString, sizeof(reportedPropertiesString), ReportMsgTemplate, pinId + DI_PORT_OFFSET, "Success");
            }

            free(cmdPayload);
        } else {
err_value:
            //init value error
            strcpy(deviceMethodResponse, "\"Illegal init value\"");
            snprintf(reportedPropertiesString, sizeof(reportedPropertiesString), ReportMsgTemplate, pinId + DI_PORT_OFFSET, "Illegal init value");
        }
    } else {
err:
        strcpy(deviceMethodResponse, "\"Error\"");
    }    
            
    // send result
    *response_size = strlen(deviceMethodResponse);
    *response = malloc(*response_size);
    if (NULL != response) {
        (void)memcpy(*response, deviceMethodResponse, *response_size);
    }
    IoT_CentralLib_SendProperty(reportedPropertiesString);

#endif  // USE_DI
end:
    return 200;
}

/// <summary>
///     Converts the IoT Hub connection status reason to a string.
/// </summary>
static const char *GetReasonString(IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
    static char *reasonString = "unknown reason";
    switch (reason) {
    case IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN:
        reasonString = "IOTHUB_CLIENT_CONNECTION_EXPIRED_SAS_TOKEN";
        break;
    case IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_DEVICE_DISABLED";
        break;
    case IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL:
        reasonString = "IOTHUB_CLIENT_CONNECTION_BAD_CREDENTIAL";
        break;
    case IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED:
        reasonString = "IOTHUB_CLIENT_CONNECTION_RETRY_EXPIRED";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_NETWORK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_NETWORK";
        break;
    case IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR:
        reasonString = "IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR";
        break;
    case IOTHUB_CLIENT_CONNECTION_OK:
        reasonString = "IOTHUB_CLIENT_CONNECTION_OK";
        break;
    case IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE:
        reasonString = "IOTHUB_CLIENT_CONNECTION_NO_PING_RESPONSE";
        break;
    }
    return reasonString;
}

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char *getAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
    switch (provisioningResult.result) {
    case AZURE_SPHERE_PROV_RESULT_OK:
        return "AZURE_SPHERE_PROV_RESULT_OK";
    case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
        return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
    case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
    default:
        return "UNKNOWN_RETURN_VALUE";
    }
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE Get_IOTHUB_DEVICE_CLIENT_LL_HANDLE(void) {
    return iothubClientHandle;
}
