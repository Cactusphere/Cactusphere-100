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
#include <stdint.h>
#include <stdarg.h>

#include <applibs/log.h>

#include "vector.h"

#include "PropertyItems.h"

void PropertyItems_AddItem(
    vector item, const char* itemName, bool isbool, ...)
{
    va_list	args;
    ResponsePropertyItem pseudo;
    pseudo.isbool = false;
    pseudo.value.b = false;
    pseudo.value.ul = 0x00;

    va_start(args, isbool);
    strncpy(pseudo.propertyName, itemName, PROPERTY_NAME_MAX_LEN);
    if (isbool) {
        pseudo.isbool = isbool;
        pseudo.value.b = (bool)va_arg(args, int);
    } else {
        pseudo.value.ul = va_arg(args, uint32_t);
    }
    va_end(args);
    vector_add_last(item, &pseudo);
}
