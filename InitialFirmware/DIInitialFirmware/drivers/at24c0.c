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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../applibs_versions.h"
#include <applibs/log.h>
#include <applibs/i2c.h>
#include "at24c0.h"

#define AT24C0_I2C_ADDRESS              (0x50)
#define AT24C0_PAGE_WRITE_LEN           (8)
#define AT24C0_HEADER_SIZE              (1)
#define AT24C0_WRITE_PAGE_BUFFER_SIZE   (AT24C0_HEADER_SIZE + AT24C0_PAGE_WRITE_LEN)

#define AT24C0_WRITE_CYCLE_MAX_MSEC     (5)

int AT24_Read(int fd, uint8_t *buf, uint8_t offset, size_t count)
{
    int ret;

    ret = I2CMaster_WriteThenRead(fd, AT24C0_I2C_ADDRESS, &offset, AT24C0_HEADER_SIZE, buf, count);
    if (ret < 0) {
        Log_Debug("ERROR: AT24_Read: %s (%d)\n", strerror(errno), errno);
    }

    return ret;
}

int AT24_PageWrite(int fd, const uint8_t *buf, uint8_t offset, size_t count)
{
    uint8_t data[AT24C0_WRITE_PAGE_BUFFER_SIZE];
    size_t writeByte;
    size_t writeByteMax = (size_t)(AT24C0_PAGE_WRITE_LEN - (offset % AT24C0_PAGE_WRITE_LEN));

    int ret;

    if (writeByteMax > count) {
        writeByte = count;
    } else {
        writeByte = writeByteMax;
    }

    memset(data, 0, sizeof(data));
    data[0] = offset;

    memcpy(&data[AT24C0_HEADER_SIZE], buf, writeByte);
    ret = I2CMaster_Write(fd, AT24C0_I2C_ADDRESS, data, AT24C0_HEADER_SIZE + writeByte);
    if (ret < 0) {
        Log_Debug("ERROR: AT24_PageWrite: %s (%d)\n", strerror(errno), errno);
    }

    return ret;
}

int AT24_Write(int fd, const uint8_t *buf, uint8_t offset, size_t count)
{
    int ret = 0;
    int i;

    const struct timespec writeInterval = {0, AT24C0_WRITE_CYCLE_MAX_MSEC * 1000 *1000};

    while (ret < count) {
        i = AT24_PageWrite(fd, &buf[ret], (uint8_t)(offset + ret), count - (size_t)ret) - AT24C0_HEADER_SIZE;
        if (i < 0) {
            break;
        }

        nanosleep(&writeInterval, NULL);
        ret += i;
    }

    return ret;
}
