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

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#ifndef CONTAINERS_VECTOR_H
#include "vector.h"
#endif

typedef struct internal_dictionary	*dictionary;

/* Starting */
dictionary dictionary_init(size_t key_size,
    size_t value_size,
    int(*comparator)(const void *const one, const void *const two));

/* Capacity */
int dictionary_size(dictionary me);
int dictionary_is_empty(dictionary me);

/* Accessing */
int dictionary_put(dictionary me, void *key, void *value);
int dictionary_get(void *value, dictionary me, void *key);
int dictionary_contains(dictionary me, void *key);
int dictionary_remove(dictionary me, void *key);
vector	dictionary_get_keys(dictionary me);

/* Ending */
void dictionary_clear(dictionary me);
dictionary dictionary_destroy(dictionary me);

#endif  // _DICTIONARY_H_
