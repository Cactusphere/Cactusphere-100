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

#include "dictionary.h"

#include "map.h"

struct internal_dictionary {
    map	body;
    vector	keys;
    size_t	key_size;
    int(*comparator)(const void *const one, const void *const two);
};

/* Starting */
dictionary
dictionary_init(size_t key_size,
    size_t value_size,
    int(*comparator)(const void *const one, const void *const two))
{
    dictionary	newObj = (dictionary)malloc(
        sizeof(struct internal_dictionary));

    if (NULL != newObj) {
        newObj->body = map_init(key_size, value_size, comparator);
        if (NULL == newObj) {
            free(newObj);
            return NULL;
        }
        newObj->keys = vector_init(key_size);
        if (NULL == newObj->keys) {
            map_destroy(newObj->body);
            free(newObj);
            return NULL;
        }
        newObj->comparator = comparator;
        newObj->key_size   = key_size;
    }

    return newObj;
}

/* Capacity */
int
dictionary_size(dictionary me)
{
    return vector_size(me->keys);
}

int
dictionary_is_empty(dictionary me)
{
    return (0 == vector_is_empty(me->keys));
}

/* Accessing */
int
dictionary_put(dictionary me, void *key, void *value)
{
    if (map_contains(me->body, key)) {
        return map_put(me->body, key, value);
    } else {
        int	retVal = map_put(me->body, key, value);

        if (0 == retVal) {
            vector_add_last(me->keys, key);
        }
        return retVal;
    }
}

int
dictionary_get(void *value, dictionary me, void *key)
{
    return map_get(value, me->body, key);
}

int
dictionary_contains(dictionary me, void *key)
{
    return map_contains(me->body, key);
}

int
dictionary_remove(dictionary me, void *key)
{
    if (map_remove(me->body, key)) {
        const char*	keyCurs = (char*)vector_get_data(me->keys);

        for (int i = 0, n = vector_size(me->keys); i < n; ++i) {
            if (0 == me->comparator(key, keyCurs)) {
                vector_remove_at(me->keys, i);
                break;
            }
            keyCurs += me->key_size;
        }

        return 1;
    } else {
        return 0;
    }
}

vector
dictionary_get_keys(dictionary me)
{
    return me->keys;
}

/* Ending */
void
dictionary_clear(dictionary me)
{
    map_clear(me->body);
    vector_clear(me->keys);
}

dictionary
dictionary_destroy(dictionary me)
{
    map_destroy(me->body);
    vector_destroy(me->keys);
    free(me);

    return NULL;
}
