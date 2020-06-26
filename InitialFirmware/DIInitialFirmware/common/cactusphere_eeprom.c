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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <applibs/i2c.h>
#include <applibs/log.h>
#include <hw/mt3620.h>

#include "drivers/at24c0.h"
#include "cactusphere_eeprom.h"

int GetEepromProperty(eepromData *data)
{
    char buf[AT24C0_SIZE];
    int fd = -1;
    int ret;

    memset(&buf, 0, sizeof(buf));

    fd = I2CMaster_Open(MT3620_ISU1_I2C);
    if (fd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }
    ret = I2CMaster_SetBusSpeed(fd, I2C_BUS_SPEED_STANDARD);
    if (ret != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        goto out;
    }
    ret = I2CMaster_SetTimeout(fd, 100);
    if (ret != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        goto out;
    }

    ret = AT24_Read(fd, buf, 0x00, AT24C0_SIZE);
    if (ret < 0) {
        goto out;
    }

    memcpy(data->venderId, &buf[VENDER_ID_OFFSET], VENDER_ID_LEN);
    memcpy(data->productId, &buf[PRODUCT_ID_OFFSET], PRODUCT_ID_LEN);
    memcpy(data->generation, &buf[GENERATION_OFFSET], GENERATION_LEN);
    memcpy(data->revision, &buf[REVISON_OFFSET], REVISON_LEN);
    memcpy(data->function, &buf[FUNCTION_OFFSET], FUNCTION_LEN);
    memcpy(data->serial, &buf[SERIAL_OFFSET], SERIAL_LEN);
    memcpy(data->region, &buf[REGION_OFFSET], REGION_LEN);
    memcpy(data->ethernetMac, &buf[ETHERNET_MAC_OFFSET], ETHERNET_MAC_LEN);
    memcpy(data->wlanMac, &buf[WLAN_MAC_OFFSET], WLAN_MAC_LEN);
    memcpy(data->keyhash, &buf[KEYHASH_OFFSET], KEYHASH_LEN);
    memcpy(data->userspace, &buf[USERSPACE_OFFSET], USERSPACE_LEN);

out:
    if (close(fd) != 0) {
        Log_Debug("ERROR: Could not close fd I2C: %s (%d).\n", strerror(errno), errno);
    }

    return ret;
}

void setEepromString(char *dist, const char *src, size_t len)
{
    char tmp[USERSPACE_LEN * 2 + 1];
    uint32_t i, j;

    memset(&tmp, 0, sizeof(tmp));

    for(i = 0, j = (len-1); i < len; i++, j--) {
        strncpy(tmp, dist, strlen(dist));
        snprintf(dist, i * 2 + 3, "%s%02X", tmp, src[j]);
    }
}
