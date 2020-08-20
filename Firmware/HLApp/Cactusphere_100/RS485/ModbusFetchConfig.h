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

#ifndef _FETCH_CONFIG_H_
#define _FETCH_CONFIG_H_

#include <stdbool.h>

#ifndef CONTAINERS_VECTOR_H
#include "vector.h"
#endif

typedef struct ModbusFetchConfig	ModbusFetchConfig;
typedef struct _json_value	json_value;

// Initialization and cleanup
extern ModbusFetchConfig*	ModbusFetchConfig_New(void);
extern void	ModbusFetchConfig_Destroy(ModbusFetchConfig* me);

// Load Modbus RTU configuration from JSON
extern bool	ModbusFetchConfig_LoadFromJSON(ModbusFetchConfig* me,
    const json_value* json, bool desireFlg, const char* version);

// Get configuration
extern vector	ModbusFetchConfig_GetFetchItems(ModbusFetchConfig* me);
extern vector	ModbusFetchConfig_GetFetchItemPtrs(ModbusFetchConfig* me);

#endif  // _FETCH_CONFIG_H_
