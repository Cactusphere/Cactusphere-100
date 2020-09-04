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

#include "StringBuf.h"

#include "vector.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct StringBuf {
    vector	mBody;          // buffer entity
    char*	mPrintfBuf;     // aux buffer for AppendByPrintf()
    size_t	mPrintBufSize;  // size of aux buffer
};

// Initialization and cleanup
StringBuf*
StringBuf_New(void)
{
    StringBuf*	newObj = (StringBuf*)malloc(sizeof(StringBuf));

    if (NULL != newObj) {
        newObj->mBody = vector_init(sizeof(char));
        if (NULL == newObj->mBody) {
            free(newObj);
            newObj = NULL;
        } else {
            newObj->mPrintfBuf    = NULL;
            newObj->mPrintBufSize = 0;
        }
    }

    return newObj;
}

void
StringBuf_Destroy(StringBuf* me)
{
    if (NULL != me) {
        vector_destroy(me->mBody);
        if (NULL != me->mPrintfBuf) {
            free(me->mPrintfBuf);
        }
        free(me);
    }
}

void
StringBuf_Clear(StringBuf* me)
{
    vector_clear(me->mBody);
}

// Attribute
size_t
StringBuf_GetLength(StringBuf* me)
{
    return (size_t)(vector_size(me->mBody));
}

const char*
StringBuf_GetStr(StringBuf* me)
{
    return (const char*)vector_get_data(me->mBody);
}

// Append string
void
StringBuf_AppendChar(StringBuf* me, char c)
{
    if (vector_is_empty(me->mBody)) {
        vector_add_last(me->mBody, &c);
        c = '\0';
        vector_add_last(me->mBody, &c);
    } else {
        vector_add_at(me->mBody, vector_size(me->mBody) - 1, &c);
    }
}

void
StringBuf_Append(StringBuf* me, const char* str)
{
    if (! vector_is_empty(me->mBody)) {
        vector_remove_last(me->mBody);
    }
    (void)vector_add_last_multi(me->mBody, str, (int)(strlen(str) + 1));
}

void
StringBuf_AppendByPrintf(StringBuf* me, const char* fmt, ...)
{
    size_t	bufSize;
    va_list	args;

    va_start(args, fmt);
    bufSize = (size_t)vsnprintf(me->mPrintfBuf, me->mPrintBufSize, fmt, args);
    va_end(args);

    if ((size_t)bufSize >= me->mPrintBufSize) {
        if (0 == me->mPrintBufSize) {
            me->mPrintfBuf = (char*)malloc(++bufSize);
        } else {
            me->mPrintfBuf = realloc(me->mPrintfBuf, ++bufSize);
        }
        me->mPrintBufSize = bufSize;
        va_start(args, fmt);
        (void)vsnprintf(me->mPrintfBuf, me->mPrintBufSize, fmt, args);
        va_end(args);
    }

    StringBuf_Append(me, me->mPrintfBuf);
}
