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

static char* PropertyItems_ReplaceString(char* beforeStr)
{
    char *pos = beforeStr;
    char *afterStr = NULL;
    size_t count = 0;
    size_t len = 0;

    for (int i = 0; *pos != '\0'; i++) {
        if(*pos == '\"'){
            count++;
        }
        pos++;
    }

    len = strlen(beforeStr) + count + 1;
    afterStr = (char *)malloc(len);
    if (!afterStr) {
        return afterStr;
    }
    memset(afterStr, 0, len);

    for (int i = 0, j = 0; beforeStr[i] != '\0'; i++, j++) {
        if (beforeStr[i] == '\"') {
            afterStr[j] = '\\';
            j++;
        }
        afterStr[j] = beforeStr[i];
    }

    return afterStr;
}

void PropertyItems_AddItem(
    vector item, const char* itemName, PropertyType type, ...)
{
    va_list	args;
    char *value;

    ResponsePropertyItem pseudo;
    pseudo.type = type;
    pseudo.value.b = false;
    pseudo.value.ul = 0x00;
    pseudo.value.str = NULL;
    
    va_start(args, type);
    strncpy(pseudo.propertyName, itemName, PROPERTY_NAME_MAX_LEN);
    
    switch (type)
    {
    case TYPE_BOOL:
        pseudo.value.b = (bool)va_arg(args, int);
        break;
    case TYPE_NUM:
        pseudo.value.ul = va_arg(args, uint32_t);
        break;
    case TYPE_STR:
        if ((value = va_arg(args, char *)) == NULL) {
            goto err;
        }
        if ((pseudo.value.str = PropertyItems_ReplaceString(value)) == NULL) {
            goto err;
        }
        break;
    case TYPE_NULL:
        break;
    default:
        goto err;
    }

    vector_add_last(item, &pseudo);
err:
    va_end(args);
}
