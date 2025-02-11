/*******************************************************************************
* VoxelX
*
* The MIT License (MIT)
* Copyright (c) 2025 Tyson Thigpen
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*******************************************************************************/

// This is a quick and crude implementation of a dynamic array, this isn't a
// full implementation, but it works for my use case. it will like be optimized
// or removed later.

#ifndef DARRAY_H
#define DARRAY_H
#undef size

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DARRAY_INITIAL_CAPACITY 16
#define DARRAY_GROWTH_FACTOR 2

typedef struct
{
  void* data;
  size_t elementSize;
  size_t size;
  size_t capacity;
} DArray;

// Initialize array with element size
static DArray* DArrayCreate(const size_t elementSize)
{
  DArray* array = malloc(sizeof(DArray));
  if (!array) return NULL;

  array->data = malloc(DARRAY_INITIAL_CAPACITY * elementSize);
  if (!array->data)
  {
    free(array);
    return NULL;
  }

  array->elementSize = elementSize;
  array->size = 0;
  array->capacity = DARRAY_INITIAL_CAPACITY;
  return array;
}

// Free array and its data
static void DArrayFree(DArray* array)
{
  if (!array) return;
  free(array->data);
  free(array);
}

// Resize array if needed
static bool DArrayResize(DArray* array, const size_t newCapacity)
{
  if (!array) return false;

  void* newData = realloc(array->data, newCapacity * array->elementSize);
  if (!newData) return false;

  array->data = newData;
  array->capacity = newCapacity;
  return true;
}

// Push element to end of array
static bool DArrayPush(DArray* array, const void* element)
{
  if (!array || !element) return false;

  if (array->size >= array->capacity)
  {
    if (!DArrayResize(array, array->capacity * DARRAY_GROWTH_FACTOR))
    {
      return false;
    }
  }

  memcpy((char*)array->data + array->size * array->elementSize, element,
         array->elementSize);

  array->size++;
  return true;
}

// Pop element from end of array
static bool DArrayPop(DArray* array, void* outElement)
{
  if (!array || !outElement || array->size == 0) return false;

  array->size--;
  memcpy(outElement, (char*)array->data + array->size * array->elementSize,
         array->elementSize);

  return true;
}

// Get element at index
static bool DArrayGet(const DArray* array, const size_t index, void* outElement)
{
  if (!array || !outElement || index >= array->size) return false;

  memcpy(outElement, (char*)array->data + index * array->elementSize,
         array->elementSize);

  return true;
}

static bool DArraySet(DArray* array, const size_t index, const void* element)
{
  if (!array || !element) return false;

  // Resize if the index is out of the current capacity
  if (index >= array->capacity)
  {
    if (!DArrayResize(array, index + 1)) { return false; }
  }

  // Write the element to the specified index
  memcpy((char*)array->data + index * array->elementSize, element,
         array->elementSize);

  // Update the size if needed
  if (index >= array->size) { array->size = index + 1; }

  return true;
}

// Get current size
static size_t DArraySize(const DArray* array)
{
  return array ? array->size : 0;
}

// Get current capacity
static size_t DArrayCapacity(const DArray* array)
{
  return array ? array->capacity : 0;
}

// Shrink capacity to fit size
static bool DArrayShrinkToFit(DArray* array)
{
  if (!array || array->size == array->capacity) return true;
  return DArrayResize(array, array->size);
}

#endif // DARRAY_H
