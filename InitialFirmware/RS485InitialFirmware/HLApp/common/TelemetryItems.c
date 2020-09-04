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

#include <applibs/log.h>

#include "dictionary.h"
#include "json.h"
#include "vector.h"

#include "TelemetryItems.h"
#include "TelemetryItemCache.h"
#include "StringBuf.h"

// string representation of telemetry data item
typedef struct TelemetryItem {
    const char* name;
    char* value;
} TelemetryItem;

// TelemetryItems class's data members
struct TelemetryItems {
    vector     mBody; // vector of telemetry data item
    StringBuf* mSb;   // for string processing
};

// telemetry item data type dictionary element
typedef struct TelemetryItemDictElem {
    const char* itemName;	// telemetry item name
    bool        isFloat;	// whether value type is float
} TelemetryItemDictElem;

// telemetry item data type dictionary
static dictionary	sTelemetryItemDict = NULL;

// comparator function for the dictionary
static int
TelemetryItemDictComparator(const void* const one, const void* const two)
{
    return strcmp(*((char**)one), *((char**)two));
}

// Initialization and cleanup of the telemetry item data type dicitionary
void
TelemetryItems_InitDictionary(void)
{
    if (NULL == sTelemetryItemDict) {
        sTelemetryItemDict = dictionary_init(
            sizeof(char*), sizeof(TelemetryItemDictElem),
            TelemetryItemDictComparator);
    }
}

void
TelemetryItems_CleanupDictionary(void)
{
    if (NULL != sTelemetryItemDict) {
        dictionary_destroy(sTelemetryItemDict);
        sTelemetryItemDict = NULL;
    }
}

// Add and remove telemetry item data type
void
TelemetryItems_AddDictionaryElem(const char* itemName, bool isFloat)
{
    TelemetryItemDictElem	pseudo;

    pseudo.itemName = itemName;
    pseudo.isFloat  = isFloat;
    dictionary_put(sTelemetryItemDict, &itemName, &pseudo);
}

void
TelemetryItems_RemoveDictionaryElem(const char* itemName)
{
    dictionary_remove(sTelemetryItemDict, &itemName);
}

// Initialization and cleanup
TelemetryItems*
TelemetryItems_New(void)
{
    TelemetryItems* newObj = (TelemetryItems*)malloc(sizeof(TelemetryItems));

    if (newObj != NULL) {
        newObj->mBody = vector_init(sizeof(TelemetryItem));
        newObj->mSb   = StringBuf_New();
        if (newObj->mBody == NULL || newObj->mSb == NULL) {
            if (newObj->mBody != NULL) {
                vector_destroy(newObj->mBody);
            }
            if (newObj->mSb != NULL) {
                StringBuf_Destroy(newObj->mSb);
            }
            free(newObj);
            newObj = NULL;
        }
    }

    return newObj;
}

void
TelemetryItems_Destroy(TelemetryItems* me)
{
    if (me != NULL) {
        TelemetryItems_Clear(me);
        vector_destroy(me->mBody);
        StringBuf_Destroy(me->mSb);
        free(me);
    }
}

// Attribute
int
TelemetryItems_Count(const TelemetryItems* me)
{
    return vector_size(me->mBody);
}

// Add and remove telemetry data item
void
TelemetryItems_Add(TelemetryItems* me, const char* name, const char* value)
{
    TelemetryItem telemetryItem;
    char* newValue = strdup(value);

    telemetryItem.name  = name;
    telemetryItem.value = newValue;
    vector_add_last(me->mBody, &telemetryItem);
}

void
TelemetryItems_Clear(TelemetryItems* me) {
    TelemetryItem* tempP = (TelemetryItem*)vector_get_data(me->mBody);

    for (int i = 0, n = vector_size(me->mBody); i < n; i++) {
        free(tempP[i].value);
    }
    vector_clear(me->mBody);
}

// Mutual conversion between cache elem
TelemetryCacheElem*
TelemetryItems_ConvToCacheElemAt(
    const TelemetryItems* me, int index, TelemetryCacheElem* outCacheElem)
{
    // Search telemetry item data type dictionary and convert from 
    // string to number according to data type
    TelemetryItem*	item = (TelemetryItem*)vector_get_data(me->mBody) + index;
    TelemetryItemDictElem	dictElem;

    if (! dictionary_get(&dictElem, sTelemetryItemDict, &item->name)) {
        return NULL;
    }

    outCacheElem->itemName = item->name;
    if (dictElem.isFloat) {
        outCacheElem->value.f = (float)atof(item->value);
    } else {
        outCacheElem->value.ul = strtoul(item->value, NULL, 10);
    }

    return outCacheElem;
}

void
TelemetryItems_AddFromCacheElem(TelemetryItems* me,
    const TelemetryCacheElem* cacheElem)
{
    // Search telemetry item data type dictionary and convert from 
    // number to string according to data type, then add to self
    TelemetryItemDictElem	dictElem;

    if (! dictionary_get(&dictElem, sTelemetryItemDict,
            &((TelemetryCacheElem*)cacheElem)->itemName)) {
        return;  // not found; error
    }

    StringBuf_Clear(me->mSb);
    if (dictElem.isFloat) {
        StringBuf_AppendByPrintf(me->mSb, "%f", cacheElem->value.f);
    } else {
        StringBuf_AppendByPrintf(me->mSb, "%lu", cacheElem->value.ul);
    }
    TelemetryItems_Add(me, cacheElem->itemName, StringBuf_GetStr(me->mSb));
}

// Convert to JSON text
const char*
TelemetryItems_ToJson(TelemetryItems* me)
{
    StringBuf_Clear(me->mSb);
    StringBuf_AppendChar(me->mSb, '{');
    for (int i = 0, n = vector_size(me->mBody); i < n; i++) {
        TelemetryItem tmp;
        vector_get_at(&tmp, me->mBody, i);
        StringBuf_AppendByPrintf(me->mSb, "\"%s\":%s", tmp.name, tmp.value);
        if (n - 1 > i) {
            StringBuf_AppendChar(me->mSb, ',');
        }
    }
    StringBuf_AppendChar(me->mSb, '}');

    return StringBuf_GetStr(me->mSb);
}

// Convert from JSON text
bool
TelemetryItems_LoadFromJson(TelemetryItems* me, const char* jsonStr)
{
    json_value* jsonObj = json_parse(jsonStr, strlen(jsonStr));

    if (NULL == jsonObj) {
        return false;
    } else if (json_object != jsonObj->type) {
        json_value_free(jsonObj);
        return false;
    } else {
        json_object_entry*  curs = jsonObj->u.object.values;
        json_object_entry*  end  = curs + jsonObj->u.object.length;
        TelemetryItemDictElem   dictElem;

        TelemetryItems_Clear(me);
        for (; curs < end; ++curs) {
            if (! dictionary_get(&dictElem, sTelemetryItemDict, &curs->name)) {
                goto err;  // unkown item
            }

            StringBuf_Clear(me->mSb);
            switch (curs->value->type) {
            case json_integer:
                StringBuf_AppendByPrintf(
                    me->mSb, "%lu", (uint32_t)curs->value->u.integer);
                break;
            case json_double:
                StringBuf_AppendByPrintf(
                    me->mSb, "%f", (float)curs->value->u.dbl);
                break;
            default:
                goto err;  // unexpected type
            }
            TelemetryItems_Add(me, dictElem.itemName, StringBuf_GetStr(me->mSb));
        }

        return true;
err:
        json_value_free(jsonObj);
        return false;
    }
}
