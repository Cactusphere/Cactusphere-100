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

#pragma once
#include <unistd.h>
#include <stdint.h>

#define VENDER_ID_OFFSET        (0x00)
#define VENDER_ID_LEN           (2)

#define PRODUCT_ID_OFFSET       (0x02)
#define PRODUCT_ID_LEN          (2)

#define GENERATION_OFFSET       (0x04)
#define GENERATION_LEN          (1)

#define REVISON_OFFSET          (0x05)
#define REVISON_LEN             (1)

#define FUNCTION_OFFSET         (0x06)
#define FUNCTION_LEN            (1)

#define SERIAL_OFFSET           (0x08)
#define SERIAL_LEN              (6)

#define REGION_OFFSET           (0x0e)
#define REGION_LEN              (2)

#define ETHERNET_MAC_OFFSET     (0x10)
#define ETHERNET_MAC_LEN        (6)

#define WLAN_MAC_OFFSET         (0x18)
#define WLAN_MAC_LEN            (6)

#define KEYHASH_OFFSET          (0x20)
#define KEYHASH_LEN             (32)

#define USERSPACE_OFFSET        (0x60)
#define USERSPACE_LEN           (32)

typedef struct {
    unsigned char venderId[VENDER_ID_LEN];
    unsigned char productId[PRODUCT_ID_LEN];
    unsigned char generation[GENERATION_LEN];
    unsigned char revision[REVISON_LEN];
    unsigned char function[FUNCTION_LEN];
    unsigned char serial[SERIAL_LEN];
    unsigned char region[REGION_LEN];
    unsigned char ethernetMac[ETHERNET_MAC_LEN];
    unsigned char wlanMac[WLAN_MAC_LEN];
    unsigned char keyhash[KEYHASH_LEN];
    unsigned char userspace[USERSPACE_LEN];
} eepromData;

int GetEepromProperty(eepromData *data);
void setEepromString(char *dist, const char *src, size_t len);
