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
#include <ctype.h>
#include <getopt.h>

#include <sys/timerfd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/networking.h>
#include <applibs/gpio.h>
#include <applibs/storage.h>
#include <applibs/i2c.h>
#include <applibs/sysevent.h>

#include <hw/mt3620.h>

#include "eventloop_timer_utilities.h"

// Azure IoT SDK
#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>
#include <iothub_security_factory.h>

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

    /* ExitCode 11-16 no longer used */

    ExitCode_SetUpSysEvent_RegisterEvent = 17,

    ExitCode_UpdateCallback_UnexpectedEvent = 18,
    ExitCode_UpdateCallback_GetUpdateEvent = 19,
    ExitCode_UpdateCallback_DeferEvent = 20,
    ExitCode_UpdateCallback_FinalUpdate = 21,
    ExitCode_UpdateCallback_UnexpectedStatus = 22,

    ExitCode_Main_SetEnv = 23,

    ExitCode_Validate_ConnectionType = 24,
    ExitCode_Validate_ScopeId = 25,
    ExitCode_Validate_IotHubHostname = 26,

    ExitCode_NW_GetInterfaceConnectionStatus_Failed = 27,
    ExitCode_NW_SetHWAddress_Failed = 28,
    ExitCode_NW_InitInterfaceList_Failed = 29,
    ExitCode_NW_GetInterfaceCount_Failed = 30,
    ExitCode_NW_GetInterfaces_Failed = 31,
    ExitCode_NW_IsNetworkingReady_Failed = 32,
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
#endif // PRODUCT_ATMARK_TECHNO_RS485

#ifdef USE_MODBUS
#include "ModbusConfigMgr.h"
#include "ModbusFetchConfig.h"
#include "LibModbus.h"
#include "ModbusDataFetchScheduler.h"
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

/// <summary>
/// Connection types to use when connecting to the Azure IoT Hub.
/// </summary>
typedef enum {
    ConnectionType_NotDefined = 0,
    ConnectionType_DPS = 1,
    ConnectionType_Direct = 2
} ConnectionType;

/// <summary>
/// Authentication state of the client with respect to the Azure IoT Hub.
/// </summary>
typedef enum {
    /// <summary>Client is not authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_NotAuthenticated = 0,
    /// <summary>Client has initiated authentication to the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_AuthenticationInitiated = 1,
    /// <summary>Client is authenticated by the Azure IoT Hub.</summary>
    IoTHubClientAuthenticationState_Authenticated = 2
} IoTHubClientAuthenticationState;

/// <summary>
/// Information such as the connection status to network and the reading status of EEPROM.
/// (If even one is disabled, the Status LED will not turn on.)
/// </summary>
typedef struct {
    bool isEepromReadSuccess;
    bool isEepromDataValid;
    IoTHubClientAuthenticationState IoTHubClientAuthState; // Authentication state with respect to the Azure IoT Hub.
    bool isNetworkConnected;
    bool isPropertySettingValid;
} SphereStatus;
SphereStatus sphereStatus = {false,false,IoTHubClientAuthenticationState_NotAuthenticated,false,false};

// Azure IoT definitions.
static char *scopeId = NULL;                                      // ScopeId for DPS.
static char *hubHostName = NULL;                                  // Azure IoT Hub Hostname.
static bool isConnectionTypeInCmdArgs = false;
static ConnectionType connectionType = ConnectionType_NotDefined; // Type of connection to use.

static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int keepalivePeriodSeconds = 20;
static bool iothubFirstConnected = false;
static const int deviceIdForDaaCertUsage = 1; // A constant used to direct the IoT SDK to use
                                              // the DAA cert under the hood.

// Application update events are received via an event loop.
static EventRegistration *updateEventReg = NULL;

static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context);
static const char *EventStatusToString(SysEvent_Status status);
static const char *UpdateTypeToString(SysEvent_UpdateType updateType);

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

// Network Interface
static ExitCode InitNetworkInterfaces(void);

// Status LED
typedef enum {
    LED_OFF = 0,
    LED_ON,
    LED_BLINK,
} LED_Status;
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
static ExitCode ValidateUserConfiguration(void);
static void ParseCommandLineArguments(int argc, char *argv[]);
static bool SetupAzureIoTHubClientWithDaa(void);
static bool SetupAzureIoTHubClientWithDps(void);
static bool ChangeLedStatus(LED_Status led_status);

typedef struct
{
    const char name[20];
    char value[20];
} EventMsgData;

eepromData eeprom;

typedef struct
{
    char starttime[10];
    char endtime[10];
    char timezone[10];
} DeferredUpdateConfig;

DeferredUpdateConfig osUpdate = {0};
DeferredUpdateConfig fwUpdate = {0};
static bool updateDeferring = false;
// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "DPS connection type: \"CmdArgs\": [\"--ScopeID\", \"<scope_id>\"]\n"
    "Direction connection type: \"CmdArgs\": [\"--Hostname\", \"<azureiothub_hostname>\"]\n";

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
    return (IoTHubClientAuthenticationState_Authenticated == sphereStatus.IoTHubClientAuthState) ? true : false;
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
/// Change the LED status.
/// </summary>
/// <returns>Returns false if the LED cannot be turned on.</returns>
static bool ChangeLedStatus(LED_Status led_status)
{
    bool ret = true;
    switch (led_status)
    {
    case LED_OFF:
        gLedState = LED_OFF;
        break;
    case LED_BLINK:
        gLedState = LED_BLINK;
        break;
    case LED_ON:
        /// If even one value of "sphereStatus" is invalid, the LED cannot turn on.
        if (sphereStatus.isEepromDataValid &&
            sphereStatus.isNetworkConnected &&
            sphereStatus.isEepromReadSuccess &&
            sphereStatus.isPropertySettingValid &&
            (IoTHubClientAuthenticationState_Authenticated == sphereStatus.IoTHubClientAuthState)) {
            gLedState = LED_ON;
        }
        else {
            gLedState = LED_BLINK;
            ret = false;
        }
        break;
    default:
        ret = false;
        break;
    }
    return ret;
}

/// <summary>
/// Set up the network interface and get information.
/// </summary>
/// <returns>ExitCode_Success if all succeeds; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode InitNetworkInterfaces(void)
{
    Networking_NetworkInterface *networkInterfacesList = NULL;
    ssize_t actualNetworkInterfaceCount = 0;

    ssize_t count = Networking_GetInterfaceCount();
    if (count == -1) {
        Log_Debug("ERROR: Networking_GetInterfaceCount: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_NW_GetInterfaceCount_Failed;
    }

    // Read current status of all interfaces.
    size_t bytesRequired = ((size_t)count) * sizeof(Networking_NetworkInterface);
    networkInterfacesList = malloc(bytesRequired);
    if (!networkInterfacesList) {
        Log_Debug("ERROR: Network Interface List malloc\n");
        return ExitCode_NW_InitInterfaceList_Failed;
    }
    actualNetworkInterfaceCount = Networking_GetInterfaces(networkInterfacesList, (size_t)count);
    if (actualNetworkInterfaceCount == -1) {
        Log_Debug("ERROR: Networking_GetInterfaces: errno=%d (%s)\n", errno, strerror(errno));
        return ExitCode_NW_GetInterfaces_Failed;
    }
    Log_Debug("INFO: NetworkInterfaceCount=%zd\n", actualNetworkInterfaceCount);

    // Set Ethernet hardware address.
    for (ssize_t i = 0; i < actualNetworkInterfaceCount; ++i) {
        if (strncmp(networkInterfacesList[i].interfaceName, "eth0", 4) == 0) {
            static Networking_Interface_HardwareAddress ha;
            for (int i = 0; i < HARDWARE_ADDRESS_LENGTH; i++) {
                ha.address[i] = eeprom.ethernetMac[HARDWARE_ADDRESS_LENGTH - (i + 1)];
            }
            if (Networking_SetHardwareAddress("eth0", ha.address, HARDWARE_ADDRESS_LENGTH) < 0) {
                Log_Debug("ERROR: setting hardware address (eth0) %d\n", errno);
                return ExitCode_NW_SetHWAddress_Failed;
            }
        }
    }

    free(networkInterfacesList);

    return ExitCode_Success;
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

    Log_Debug("Getting EEPROM information.\n");
    err = GetEepromProperty(&eeprom);
    if (err < 0) {
        ct_error = -EEPROM_READ_ERROR;
        sphereStatus.isEepromReadSuccess = false;
        ChangeLedStatus(LED_BLINK);
    }
    else {
        sphereStatus.isEepromReadSuccess = true;
    }

    exitCode = InitNetworkInterfaces();
    if (exitCode != ExitCode_Success) {
        return exitCode;
    }

    ParseCommandLineArguments(argc, argv);
    exitCode = ValidateUserConfiguration();
    if (exitCode != ExitCode_Success) {
        return exitCode;
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

    // Check whether the network is up.
    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1) {
        if (isNetworkReady) {
            if (sphereStatus.IoTHubClientAuthState == IoTHubClientAuthenticationState_NotAuthenticated) {
                SetupAzureClient();
                IoT_CentralLib_Initialize(CACHE_BUF_SIZE, false);
            }
            sphereStatus.isNetworkConnected = true;
            ChangeLedStatus(LED_ON);
        } else {
            sphereStatus.isNetworkConnected = false;
            ChangeLedStatus(LED_BLINK);
        }
    } else {
        Log_Debug("ERROR: Networking_IsNetworkingReady: %d (%s)\n", errno,
                  strerror(errno));
        exitCode = ExitCode_NW_IsNetworkingReady_Failed;
        return;
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
    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

/// <summary>
///     Parse the command line arguments given in the application manifest.
/// </summary>
static void ParseCommandLineArguments(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {{"ConnectionType", required_argument, NULL, 'c'},
                                                   {"ScopeID", required_argument, NULL, 's'},
                                                   {"Hostname", required_argument, NULL, 'h'},
                                                   {NULL, 0, NULL, 0}};

    if (argc == 2) {
        scopeId = argv[1];
        connectionType = ConnectionType_DPS;
        Log_Debug("ScopeID: %s\n", scopeId);
    } else {
        // Loop over all of the options
        while ((option = getopt_long(argc, argv, "c:s:h:d:", cmdLineOptions, NULL)) != -1) {
            // Check if arguments are missing. Every option requires an argument.
            if (optarg != NULL && optarg[0] == '-') {
                Log_Debug("Warning: Option %c requires an argument\n", option);
                continue;
            }
            switch (option) {
            case 'c':
                Log_Debug("ConnectionType: %s\n", optarg);
                isConnectionTypeInCmdArgs = true;
                if (strcmp(optarg, "DPS") == 0) {
                    connectionType = ConnectionType_DPS;
                } else if (strcmp(optarg, "Direct") == 0) {
                    connectionType = ConnectionType_Direct;
                }
                break;
            case 's':
                Log_Debug("ScopeID: %s\n", optarg);
                scopeId = optarg;
                connectionType = isConnectionTypeInCmdArgs ? connectionType : ConnectionType_DPS;
                break;
            case 'h':
                Log_Debug("Hostname: %s\n", optarg);
                hubHostName = optarg;
                connectionType = isConnectionTypeInCmdArgs ? connectionType : ConnectionType_Direct;
                break;
            default:
                // Unknown options are ignored.
                break;
            }
        }
    }
}

/// <summary>
///     Validates if the values of the Scope ID, IotHub Hostname and Device ID were set.
/// </summary>
/// <returns>ExitCode_Success if the parameters were provided; otherwise another
/// ExitCode value which indicates the specific failure.</returns>
static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    if (connectionType < ConnectionType_DPS || connectionType > ConnectionType_Direct) {
        validationExitCode = ExitCode_Validate_ConnectionType;
    }
    
    if (!isConnectionTypeInCmdArgs && scopeId != NULL && hubHostName != NULL){
        connectionType = ConnectionType_DPS;
    }

    if (connectionType == ConnectionType_DPS) {
        if (scopeId == NULL) {
            validationExitCode = ExitCode_Validate_ScopeId;
        } else {
            Log_Debug("Using DPS Connection: Azure IoT DPS Scope ID %s\n", scopeId);
        }
    }

    if (connectionType == ConnectionType_Direct) {
        if (hubHostName == NULL) {
            validationExitCode = ExitCode_Validate_IotHubHostname;
        }
        if (validationExitCode == ExitCode_Success) {
            Log_Debug("Using Direct Connection: Azure IoT Hub Hostname %s\n", hubHostName);
        }
    }

    if (validationExitCode != ExitCode_Success) {
        Log_Debug("Command line arguments for application shoud be set as below\n%s",
                  cmdLineArgsUsageText);
    }
    return validationExitCode;
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
///     This function matches the SysEvent_EventsCallback signature, and is invoked 
///     from the event loop when the system wants to perform an application or system update.
///     See <see cref="SysEvent_EventsCallback" /> for information about arguments.
/// </summary>
static void UpdateCallback(SysEvent_Events event, SysEvent_Status status, const SysEvent_Info *info,
                           void *context)
{
    if (event != SysEvent_Events_UpdateReadyForInstall) {
        Log_Debug("ERROR: unexpected event: 0x%x\n", event);
        exitCode = ExitCode_UpdateCallback_UnexpectedEvent;
        return;
    }

    // Print information about received message.
    Log_Debug("INFO: Status: %s (%u)\n", EventStatusToString(status), status);

    int result;

    SysEvent_Info_UpdateData data;
    DeferredUpdateConfig deferTime;
    static char updateInfo[128] = { 0 };
    uint32_t timeStamp = IoT_CentralLib_GetTmeStamp();

    switch (status) {
        // If an update is pending, and the user has not allowed updates, then defer the update.
    case SysEvent_Status_Pending:
        result = SysEvent_Info_GetUpdateData(info, &data);

        if (result == -1) {
            Log_Debug("ERROR: SysEvent_Info_GetUpdateData failed: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_UpdateCallback_GetUpdateEvent;
            return;
        }

        if (data.update_type == SysEvent_UpdateType_App && strlen(fwUpdate.starttime)
            && strlen(fwUpdate.endtime) && strlen(fwUpdate.timezone) ) {
            deferTime = fwUpdate;
        } else if (data.update_type == SysEvent_UpdateType_System && strlen(osUpdate.starttime)
                 && strlen(osUpdate.endtime) && strlen(osUpdate.timezone) ) {
            deferTime = osUpdate;
        } else if (updateDeferring == false){
            result = SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall, 1);
            if (result == -1) {
                Log_Debug("ERROR: SysEvent_DeferEvent: %s (%d).\n", strerror(errno), errno);
                exitCode = ExitCode_UpdateCallback_DeferEvent;
            }
            return;
        } else {
            return;
        }
        Log_Debug("INFO: Update Type: %s (%u).\n", UpdateTypeToString(data.update_type),
                data.update_type);
        result = setenv("TZ", deferTime.timezone, 1);
        if (result == -1) {
            Log_Debug("ERROR: setenv failed with error code: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_Main_SetEnv;
        }
        int deferMin = 0;
        char timeBuf[64];
        time_t t, startTime_t, endTime_t;
        time(&t);
        struct tm tm = *localtime(&t);
        if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T %Z", &tm) != 0) {
                Log_Debug("INFO: Received update event: %s\n", timeBuf);
        }

        struct tm startTm = tm;
        struct tm endTm = tm;
        int hour, min;
        sscanf(deferTime.starttime, "%d:%d", &hour, &min);
        startTm.tm_hour = hour;
        startTm.tm_min = min;
        startTime_t = mktime(&startTm);
        if (t > startTime_t) {
            sscanf(deferTime.endtime, "%d:%d", &hour, &min);
            endTm.tm_hour = hour;
            endTm.tm_min = min;
            endTime_t = mktime(&endTm);
            if (t < endTime_t) {
                break;
            }
            startTm.tm_mday += 1;
            startTime_t = mktime(&startTm);
        }
        deferMin = (int)(difftime(startTime_t, t) / 60);
        if (deferMin == 0) {
            return;
        }

        result = SysEvent_DeferEvent(SysEvent_Events_UpdateReadyForInstall, deferMin);
        if (result == -1) {
            Log_Debug("ERROR: SysEvent_DeferEvent: %s (%d).\n", strerror(errno), errno);
            exitCode = ExitCode_UpdateCallback_DeferEvent;
        }
        Log_Debug("INFO: Deferring update for %d minute.\n", deferMin);
        if (strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %T %Z", &startTm) != 0) {
            Log_Debug("INFO: Update scheduled: %s\n", timeBuf);
            snprintf(updateInfo, sizeof(updateInfo),
            "{ \"UpdateInformation\": \"Update scheduled %s.\" }", timeBuf);
            IoT_CentralLib_SendTelemetry(updateInfo, &timeStamp);
        }
        break;

    case SysEvent_Status_Final:
        Log_Debug("INFO: Final update. App will update in 10 seconds.\n");
        result = SysEvent_Info_GetUpdateData(info, &data);
        if (result != -1) {
            snprintf(updateInfo, sizeof(updateInfo),
            "{ \"UpdateInformation\": \"%s updated.\" }", UpdateTypeToString(data.update_type));
            IoT_CentralLib_SendTelemetry(updateInfo, &timeStamp);
            IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
        }
        // Terminate app before it is forcibly shut down and replaced.
        // The application may be restarted before the update is applied.
        exitCode = ExitCode_UpdateCallback_FinalUpdate;
        break;

    case SysEvent_Status_Deferred:
        Log_Debug("INFO: Update deferred.\n");
        break;

    case SysEvent_Status_Complete:
    default:
        Log_Debug("ERROR: Unexpected status %d.\n", status);
        exitCode = ExitCode_UpdateCallback_UnexpectedStatus;
        break;
    }

    Log_Debug("\n");
}

/// <summary>
///     Convert the supplied system event status to a human-readable string.
/// </summary>
/// <param name="status">The status.</param>
/// <returns>String representation of the supplied status.</param>
static const char *EventStatusToString(SysEvent_Status status)
{
    switch (status) {
    case SysEvent_Status_Invalid:
        return "Invalid";
    case SysEvent_Status_Pending:
        return "Pending";
    case SysEvent_Status_Final:
        return "Final";
    case SysEvent_Status_Deferred:
        return "Deferred";
    case SysEvent_Status_Complete:
        return "Completed";
    default:
        return "Unknown";
    }
}

/// <summary>
///     Convert the supplied update type to a human-readable string.
/// </summary>
/// <param name="updateType">The update type.</param>
/// <returns>String representation of the supplied update type.</param>
static const char *UpdateTypeToString(SysEvent_UpdateType updateType)
{
    switch (updateType) {
    case SysEvent_UpdateType_Invalid:
        return "Invalid";
    case SysEvent_UpdateType_App:
        return "Application";
    case SysEvent_UpdateType_System:
        return "System";
    default:
        return "Unknown";
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

    updateEventReg = SysEvent_RegisterForEventNotifications(
        eventLoop, SysEvent_Events_UpdateReadyForInstall, UpdateCallback, NULL);
    if (updateEventReg == NULL) {
        Log_Debug("ERROR: could not register update event: %s (%d).\n", strerror(errno), errno);
        return ExitCode_SetUpSysEvent_RegisterEvent;
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
    ChangeLedStatus(LED_ON);

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

    SysEvent_UnregisterForEventNotifications(updateEventReg);

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
    Log_Debug("Azure IoT connection status: %s\n", GetReasonString(reason));

    if (result != IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        sphereStatus.IoTHubClientAuthState = IoTHubClientAuthenticationState_NotAuthenticated;
        ChangeLedStatus(LED_BLINK);
        return;
    }

    sphereStatus.IoTHubClientAuthState = IoTHubClientAuthenticationState_Authenticated;

    Log_Debug("IoT Hub Authenticated: %s\n", GetReasonString(reason));

    if (reason == IOTHUB_CLIENT_CONNECTION_OK) {
        if(!iothubFirstConnected) {
            cactusphere_error_notify(FIRST_CONNECT_IOTC);
            iothubFirstConnected = true;
        } else {
            cactusphere_error_notify(RE_CONNECT_IOTC);
        }

        if (ct_error == -EEPROM_READ_ERROR) {
            sphereStatus.isEepromReadSuccess = false;
            ChangeLedStatus(LED_BLINK);
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

            sphereStatus.isEepromReadSuccess = true;
            ChangeLedStatus(LED_ON);

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
                sphereStatus.isEepromDataValid = false;
                ChangeLedStatus(LED_BLINK);
                cactusphere_error_notify(ILLEGAL_PACKAGE);
                exitCode = ExitCode_TermHandler_SigTerm;
            }
            else {
                sphereStatus.isEepromDataValid = true;
            }
        }
    } else {
        ChangeLedStatus(LED_BLINK);
    }
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
static void SetupAzureClient(void)
{
    bool isAzureClientSetupSuccessful = false;

    if (iothubClientHandle != NULL)
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);

    if (connectionType == ConnectionType_Direct) {
        isAzureClientSetupSuccessful = SetupAzureIoTHubClientWithDaa();
    } else if (connectionType == ConnectionType_DPS) {
        isAzureClientSetupSuccessful = SetupAzureIoTHubClientWithDps();
    }

    if (!isAzureClientSetupSuccessful) {
        ChangeLedStatus(LED_BLINK);

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

    // Set client authentication state to initiated. This is done to indicate that
    // SetUpAzureIoTHubClient() has been called (and so should not be called again) while the
    // client is waiting for a response via the ConnectionStatusCallback().
    sphereStatus.IoTHubClientAuthState = IoTHubClientAuthenticationState_AuthenticationInitiated;

    ChangeLedStatus(LED_ON);

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
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DAA
/// </summary>
static bool SetupAzureIoTHubClientWithDaa(void)
{
    // Set up auth type
    int retError = iothub_security_init(IOTHUB_SECURITY_TYPE_X509);
    if (retError != 0) {
        Log_Debug("ERROR: iothub_security_init failed with error %d.\n", retError);
        return false;
    }

    // Create Azure Iot Hub client handle
    iothubClientHandle =
        IoTHubDeviceClient_LL_CreateWithAzureSphereFromDeviceAuth(hubHostName, MQTT_Protocol);

    if (iothubClientHandle == NULL) {
        Log_Debug("IoTHubDeviceClient_LL_CreateFromDeviceAuth returned NULL.\n");
        return false;
    }

    // Enable DAA cert usage when x509 is invoked
    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, "SetDeviceId",
                                        &deviceIdForDaaCertUsage) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"SetDeviceId\".\n");
        return false;
    }

    return true;
}

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     with DPS
/// </summary>
static bool SetupAzureIoTHubClientWithDps(void)
{
    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
                                                                          &iothubClientHandle);
    Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
              getAzureSphereProvisioningResultString(provResult));

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {
        return false;
    }

    return true;
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
    static const char* EventMsgTemplate_null = "{ \"%s\": null }";
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
            case TYPE_NULL:
                propertyStr_len = strlen(curs->propertyName) + JSON_FORMAT_NUM;
                propertyStr = (char *)malloc(propertyStr_len);
                if (propertyStr) {
                    memset(propertyStr, 0, propertyStr_len);
                    snprintf(propertyStr, propertyStr_len, EventMsgTemplate_null,
                             curs->propertyName, curs->value.ul);
                }
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

static void ParseDeferredUpdateConfig(json_value* json,
    DeferredUpdateConfig* config, const char* key, vector item)
{
    if (! json) {
        return;
    }

    if (json->type != json_string) {
        json = json_GetKeyJson("value", json);
    }
    PropertyItems_AddItem(item, key, TYPE_STR, json->u.string.ptr);
    json = json_parse(json->u.string.ptr, json->u.string.length);

    for (unsigned int i = 0; i < json->u.object.length; i++) {
        char* propertyName = json->u.object.values[i].name;
        json_value* item = json->u.object.values[i].value;

        if (0 == strcmp(propertyName, "start")) {
            strncpy(config->starttime, item->u.string.ptr, item->u.string.length);
        } else if (0 == strcmp(propertyName, "end")) {
            strncpy(config->endtime, item->u.string.ptr, item->u.string.length);
        } else if (0 == strcmp(propertyName, "timezone")) {
             strncpy(config->timezone, item->u.string.ptr, item->u.string.length);
        }
    }
}

static bool CheckDeferredUpdateConfig(const unsigned char* payload,
    unsigned int payloadSize, vector item)
{
    json_value* jsonObj = json_parse(payload, payloadSize);
    json_value* desiredObj = NULL;
    json_value* osUpdateObj = NULL;
    json_value* fwUpdateObj = NULL;
    bool ret = false;
    bool doResume = false;

    updateDeferring = true;
    Log_Debug("payload=%s", payload);
    desiredObj = json_GetKeyJson("desired", jsonObj);
    if (desiredObj == NULL) {
        osUpdateObj = json_GetKeyJson("OSUpdateTime", jsonObj);
        fwUpdateObj = json_GetKeyJson("FWUpdateTime", jsonObj);
    } else {
        osUpdateObj = json_GetKeyJson("OSUpdateTime", desiredObj);
        fwUpdateObj = json_GetKeyJson("FWUpdateTime", desiredObj);
    }

    if (osUpdateObj != NULL) {
        if (osUpdateObj->type == json_null) {
            memset(&osUpdate, 0, sizeof(DeferredUpdateConfig));
            doResume = true;
        } else {
            ParseDeferredUpdateConfig(osUpdateObj, &osUpdate, "OSUpdateTime", item);
        }
        ret = true;
    }

    if (fwUpdateObj != NULL) {
        if (fwUpdateObj->type == json_null) {
            memset(&fwUpdate, 0, sizeof(DeferredUpdateConfig));
            doResume = true;
        } else {
            ParseDeferredUpdateConfig(fwUpdateObj, &fwUpdate, "FWUpdateTime", item);
        }
        ret = true;
    }

    if (doResume) {
        SysEvent_ResumeEvent(SysEvent_Events_UpdateReadyForInstall);
    }

    return ret;
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

    bool defupderr = CheckDeferredUpdateConfig(payload, payloadSize, Send_PropertyItem);

#ifdef USE_MODBUS
    SphereWarning err = ModbusConfigMgr_LoadAndApplyIfChanged(payload, payloadSize, Send_PropertyItem);
    if (defupderr && err == UNSUPPORTED_PROPERTY) {
        err = NO_ERROR;
    }
    switch (err)
    {
    case NO_ERROR:
    case ILLEGAL_PROPERTY:
        DataFetchScheduler_Init(
            mTelemetrySchedulerArr[MODBUS_RTU],
            ModbusFetchConfig_GetFetchItemPtrs(ModbusConfigMgr_GetModbusFetchConfig()));

        if (err == NO_ERROR) {
            sphereStatus.isPropertySettingValid = true;
            ChangeLedStatus(LED_ON);
        } else { // ILLEGAL_PROPERTY
            // do not set ct_error and exitCode,
            // as it won't hang with this error.
            Log_Debug("ERROR: Receive illegal property.\n");
            sphereStatus.isPropertySettingValid = false;
            ChangeLedStatus(LED_BLINK);
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
    if (defupderr && err == UNSUPPORTED_PROPERTY) {
        err = NO_ERROR;
    }
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
            sphereStatus.isPropertySettingValid = true;
            ChangeLedStatus(LED_ON);
        } else { // ILLEGAL_PROPERTY
            // do not set ct_error and exitCode,
            // as it won't hang with this error.
            Log_Debug("ERROR: Receive illegal property.\n");
            sphereStatus.isPropertySettingValid = false;
            ChangeLedStatus(LED_BLINK);
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
        sphereStatus.isPropertySettingValid = false;
        ChangeLedStatus(LED_BLINK);
        cactusphere_error_notify(err);
        exitCode = ExitCode_TermHandler_SigTerm;
    }
}

static int CommandCallback(const char* method_name, const unsigned char* payload, size_t size,
    unsigned char** response, size_t* response_size, void* userContextCallback) {

    if (ct_error < 0) {
        goto end;
    }

    char deviceMethodResponse[100];
    char reportedPropertiesString[100];

#ifdef USE_MODBUS
    static const char* ReportMsgTemplate = "{ \"ModbusWriteRegisterResult\": \"%s\" }";

    ModbusOneshotcommand(payload, size, deviceMethodResponse);

    // send result
    *response_size = strlen(deviceMethodResponse);
    *response = malloc(*response_size);
    if (NULL != response) {
        (void)memcpy(*response, deviceMethodResponse, *response_size);
    }
    snprintf(reportedPropertiesString, sizeof(reportedPropertiesString), ReportMsgTemplate, deviceMethodResponse);
    IoT_CentralLib_SendProperty(reportedPropertiesString);
#endif

#ifdef USE_DI
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
                free(cmdPayload);
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
