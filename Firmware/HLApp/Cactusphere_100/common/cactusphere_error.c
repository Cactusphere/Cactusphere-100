/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Atmark Techno, Inc.
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

#include "cactusphere_error.h"
#include "LibCloud.h"

void cactusphere_error_notify(SphereWarning ct_error)
{
    uint32_t timeStamp = IoT_CentralLib_GetTmeStamp();

    switch (ct_error)
    {
    case EEPROM_READ_ERROR:
        IoT_CentralLib_SendTelemetry("{ \"SphereWarning\": \"Cannot read EEPROM DATA.\" }", &timeStamp);
        break;
    case EEPROM_DATA_GARBLED:
        IoT_CentralLib_SendTelemetry("{ \"SphereWarning\": \"EEPROM DATA is garbled.\" }", &timeStamp);
        break;
    case ILLEGAL_PACKAGE:
        IoT_CentralLib_SendTelemetry("{ \"SphereWarning\": \"This package is illegal.\" }", &timeStamp);
        break;
    case ILLEGAL_PROPERTY:
        IoT_CentralLib_SendTelemetry("{ \"SphereWarning\": \"Check and modify property settings.\" }", &timeStamp);
        break;
    case ILLEGAL_DESIRED_PROPERTY:
        IoT_CentralLib_SendTelemetry("{ \"SphereWarning\": \"Receive illegal property settings.\" }", &timeStamp);
        break;
    case UNSUPPORTED_PROPERTY:
        IoT_CentralLib_SendTelemetry("{ \"SphereWarning\": \"Receive unsupported property.\" }", &timeStamp);
        break;

    default:
        break;
    }
}
