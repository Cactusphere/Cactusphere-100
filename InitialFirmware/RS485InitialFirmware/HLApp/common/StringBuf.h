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

#ifndef _STRING_BUF_H_
#define _STRING_BUF_H_

#ifndef _STDDEF_H
#include <stddef.h>
#endif

typedef struct StringBuf	StringBuf;

// Initialization and cleanup
extern StringBuf*	StringBuf_New(void);
extern void	StringBuf_Destroy(StringBuf* me);
extern void	StringBuf_Clear(StringBuf* me);

// Attribute
extern size_t	StringBuf_GetLength(StringBuf* me);
extern const char*	StringBuf_GetStr(StringBuf* me);

// Append string
extern void	StringBuf_AppendChar(StringBuf* me, char c);
extern void	StringBuf_Append(StringBuf* me, const char* str);
extern void	StringBuf_AppendByPrintf(StringBuf* me, const char* fmt, ...);

#endif  // _STRING_BUF_H_
