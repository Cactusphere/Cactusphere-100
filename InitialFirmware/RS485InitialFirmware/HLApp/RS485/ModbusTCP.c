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

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "ModbusTCP.h"
#include "vector.h"

# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>

#define MODBUS_TCP_HEADER_LENGTH 7
#define MODBUS_TCP_CHECKSUM_LENGTH 0

#define MIN_REQ_LENGTH 12
#define MAX_MESSAGE_LENGTH 256

#define MODBUS_TCP_PRESET_REQ_LENGTH 12

// function
#define FC_READ_HOLDING_REGISTER 0x03
#define FC_READ_INPUT_REGISTERS 0x04
#define FC_WRITE_SINGLE_REGISTER 0x06

typedef enum {
    PARSE_FUNCTION,
    PARSE_META,
    PARSE_DATA

}PARSE_STEP;

// ModbusTCP structure
typedef struct ModbusTcpCtx {
    int socket;
    uint16_t t_id;
    char ip[16];
    int port;
    int header_length;
    int checksum_length;
}ModbusTcpCtx;

static int
ModbusTCP_CreateRequestMsg(ModbusTcpCtx* me, uint8_t unitId, int function, int addr, int nb, uint8_t *req) {

    int mbap_length = MODBUS_TCP_PRESET_REQ_LENGTH - 6;

    if (me->t_id < UINT16_MAX) {
        me->t_id++;
    }
    else {
        me->t_id = 0;
    }

    req[0] = (uint8_t)(me->t_id >> 8);
    req[1] = (uint8_t)(me->t_id & 0x00ff);
    req[2] = 0;
    req[3] = 0;
    // [4][5] skip
    req[6] = unitId;
    req[7] = (uint8_t)function;
    req[8] = (uint8_t)(addr >> 8);
    req[9] = (uint8_t)(addr & 0x00ff);
    req[10] = (uint8_t)(nb >> 8);
    req[11] = (uint8_t)(nb & 0x00ff);

    req[4] = (uint8_t)(mbap_length >> 8);
    req[5] = (uint8_t)(mbap_length & 0x00FF);

    return MODBUS_TCP_PRESET_REQ_LENGTH;
}

static int 
ModbusTCP_CheckResponseMsg(ModbusTcpCtx* me, uint8_t* req, uint8_t* rsp){
    int rc = 0;
    const int offset = me->header_length;
    const int function = rsp[offset];
    int req_calc_length = 0;
    int rsp_calc_length = 0;

    if (req[0] != rsp[0] || req[1] != rsp[1]) {
        return -1;
    }

    if (rsp[2] != 0x0 && rsp[3] != 0x0) {
        return -1;
    }

    if (function >= 0x80) {
        return -1;
    }

    switch (function) {
    case FC_READ_HOLDING_REGISTER:
    case FC_READ_INPUT_REGISTERS:
        req_calc_length = (req[offset + 3] << 8) + req[offset + 4];
        rsp_calc_length = (rsp[offset + 1] / 2);
        break;
    default:
        req_calc_length = rsp_calc_length = 1;
        break;
    }

    if (req_calc_length == rsp_calc_length) {
        rc = rsp_calc_length;
    }

    return rc;
}

static int
ModbusTCP_RecieveMsg(ModbusTcpCtx* me, uint8_t* msg) {
    int rc = 0;
    int rsp_length = 0;
    int msg_length = 0;
    PARSE_STEP step = PARSE_FUNCTION;

    rsp_length = me->header_length + 1;

    while (rsp_length != 0) {
        rc = recv(me->socket, (char*)msg + msg_length, (size_t)rsp_length, 0);
        msg_length += rc;
        rsp_length -= rc;

        if (rsp_length == 0) {
            switch (step) {
            case PARSE_FUNCTION:
                if (msg[me->header_length] == FC_WRITE_SINGLE_REGISTER) {
                    rsp_length = 4;
                }
                else {
                    rsp_length = 1;
                }
                step = PARSE_META;
                break;
            case PARSE_META:
                if (msg[me->header_length] == FC_READ_HOLDING_REGISTER) {
                    rsp_length = msg[me->header_length + 1];
                }
                else {
                    rsp_length = 0;
                }
                step = PARSE_DATA;
                break;
            default:
                break;
            }
        }
    }
    return msg_length;
}

static bool
ModbusTCP_ReadRegister(ModbusTcpCtx* me, int unitId, int regAddr, int function, unsigned short* dst) {
    int rc;
    int req_length;
    uint8_t req[MIN_REQ_LENGTH];
    uint8_t rsp[MAX_MESSAGE_LENGTH];
    int length = 1;
    int sendSize = 0;

    req_length = ModbusTCP_CreateRequestMsg(me, (uint8_t)unitId, function, regAddr, length, req);
    rc = req_length;

    while (rc > 0) {
        sendSize = send(me->socket, (const char*)req + sendSize, (size_t)rc, MSG_NOSIGNAL);
        rc -= sendSize;
    }

    if (rc >= 0) {
        int offset;
        int i;

        rc = ModbusTCP_RecieveMsg(me, rsp);

        rc = ModbusTCP_CheckResponseMsg(me, req, rsp);
        if (rc == -1)
            return false;

        offset = me->header_length;

        for (i = 0; i < rc; i++) {
            dst[i] = (unsigned short)((rsp[offset + 2 + (i << 1)] << 8) |
                rsp[offset + 3 + (i << 1)]);
        }
    }

    return rc;
}

// Initialization and cleanup
ModbusTcpCtx* 
ModbusTCP_Initialize(const char* ip, int port) {
    ModbusTcpCtx* newObj;
    size_t dest_size;

    newObj = (ModbusTcpCtx*)malloc(sizeof(ModbusTcpCtx));

    newObj->socket = 0;
    newObj->port = port;
    newObj->t_id = 0;

    dest_size = sizeof(char) * 16;
    strncpy(newObj->ip, ip, dest_size);

    newObj->header_length = MODBUS_TCP_HEADER_LENGTH;
    newObj->checksum_length = MODBUS_TCP_CHECKSUM_LENGTH;

    return newObj;
}

void
ModbusTCP_Destroy(ModbusTcpCtx* me) {
    free(me);
}

// Connect
bool 
ModbusTCP_Connect(ModbusTcpCtx* me) {
    struct sockaddr_in addr;
    int rc = 0;
    int option;

    me->socket = socket(PF_INET, SOCK_STREAM, 0);
    if (me->socket == -1) {
        return false;
    }

    option = 1;
    rc = setsockopt(me->socket, IPPROTO_TCP, TCP_NODELAY,
        (const void*)&option, sizeof(int));

    if (rc == -1) {
        close(me->socket);
        me->socket = -1;
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(me->port);
    addr.sin_addr.s_addr = inet_addr(me->ip);

    rc = connect(me->socket, (struct sockaddr*)&addr, sizeof(addr));

    if (rc == -1) {
        close(me->socket);
        me->socket = -1;
        return false;
    }

    return true;
}

// Disconnect
void 
ModbusTCP_Disconnect(ModbusTcpCtx* me) {
    close(me->socket);
    me->socket = -1;
}

// Read single register
bool 
ModbusTCP_ReadSingleRegister(ModbusTcpCtx* me, int unitId, int regAddr, unsigned short* dst) {
    return ModbusTCP_ReadRegister(me, unitId, regAddr, FC_READ_HOLDING_REGISTER, dst);
}

bool
ModbusTCP_ReadSingleInputRegister(ModbusTcpCtx* me, int unitId, int regAddr, unsigned short* dst) {
    return ModbusTCP_ReadRegister(me, unitId, regAddr, FC_READ_INPUT_REGISTERS, dst);
}

// Write single register
bool
ModbusTCP_WriteSingleRegister(ModbusTcpCtx* me, int unitId, int regAddr, unsigned short value) {
    int function = FC_WRITE_SINGLE_REGISTER;
    int rc;
    int req_length;
    uint8_t req[MIN_REQ_LENGTH];
    uint8_t rsp[MAX_MESSAGE_LENGTH];
    int sendSize = 0;

    req_length = ModbusTCP_CreateRequestMsg(me, (uint8_t)unitId, function, regAddr, (int)value, req);
    rc = req_length;

    while (rc > 0) {
        sendSize = send(me->socket, (const char*)req + sendSize, (size_t)rc, MSG_NOSIGNAL);
        rc -= sendSize;
    }

    if (rc >= 0) {
        rc = ModbusTCP_RecieveMsg(me, rsp);
        rc = ModbusTCP_CheckResponseMsg(me, req, rsp);
        if (rc == -1)
            return false;
    }
    return rc;
}
