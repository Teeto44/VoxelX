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

// This is a quick and crude implementation of a map, this isn't a full
// implementation, but it works for my use case. it will like be optimized
// or removed later.

#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAP_INITIAL_CAPACITY 16
#define MAP_LOAD_FACTOR_THRESHOLD 0.85
#define MAP_GROWTH_FACTOR 2

typedef size_t (*HashFunction)(const void* key);
typedef bool (*KeyCompare)(const void* key1, const void* key2);

typedef struct MapEntry
{
  void* key;
  void* value;
  struct MapEntry* next;
  size_t hash; // Cached hash value
} MapEntry;

typedef struct Map
{
  MapEntry** buckets;
  size_t capacity;
  size_t size;
  size_t keySize;
  size_t valueSize;
  HashFunction hashFn;
  KeyCompare compareFn;
  size_t mask; // capacity - 1 for fast modulo
} Map;

static Map* MapCreate(const size_t keySize, const size_t valueSize,
                      const HashFunction hashFn, const KeyCompare compareFn)
{
  if (!hashFn || !compareFn || keySize == 0 || valueSize == 0) return NULL;
  Map* map = malloc(sizeof(Map));
  if (!map) return NULL;
  map->buckets = calloc(MAP_INITIAL_CAPACITY, sizeof(MapEntry*));
  if (!map->buckets)
  {
    free(map);
    return NULL;
  }
  map->capacity = MAP_INITIAL_CAPACITY;
  map->size = 0;
  map->keySize = keySize;
  map->valueSize = valueSize;
  map->hashFn = hashFn;
  map->compareFn = compareFn;
  map->mask = MAP_INITIAL_CAPACITY - 1;
  return map;
}

static void MapFree(Map* map)
{
  if (!map || !map->buckets) return;
  for (size_t i = 0; i < map->capacity; i++)
  {
    MapEntry* entry = map->buckets[i];
    while (entry)
    {
      MapEntry* next = entry->next;
      free(entry);
      entry = next;
    }
    map->buckets[i] = NULL;
  }
  free(map->buckets);
  free(map);
}

static bool MapResize(Map* map, const size_t newCapacity)
{
  MapEntry** newBuckets = calloc(newCapacity, sizeof(MapEntry*));
  if (!newBuckets) return false;
  const size_t newMask = newCapacity - 1;
  for (size_t i = 0; i < map->capacity; i++)
  {
    MapEntry* entry = map->buckets[i];
    while (entry)
    {
      const size_t newIndex = entry->hash & newMask;
      MapEntry* next = entry->next;
      entry->next = newBuckets[newIndex];
      newBuckets[newIndex] = entry;
      entry = next;
    }
  }
  free(map->buckets);
  map->buckets = newBuckets;
  map->capacity = newCapacity;
  map->mask = newMask;
  return true;
}

static bool MapPut(Map* map, const void* key, const void* value)
{
  if (!map || !key || !value) return false;
  if ((float)map->size / map->capacity >= MAP_LOAD_FACTOR_THRESHOLD)
  {
    if (!MapResize(map, map->capacity * MAP_GROWTH_FACTOR)) return false;
  }
  const size_t hash = map->hashFn(key);
  const size_t index = hash & map->mask;
  MapEntry* entry = map->buckets[index];
  while (entry)
  {
    if (entry->hash == hash && map->compareFn(entry->key, key))
    {
      memcpy(entry->value, value, map->valueSize);
      return true;
    }
    entry = entry->next;
  }
  const size_t totalSize = sizeof(MapEntry) + map->keySize + map->valueSize;
  uint8_t* memory = malloc(totalSize);
  if (!memory) return false;
  entry = (MapEntry*)memory;
  entry->key = memory + sizeof(MapEntry);
  entry->value = memory + sizeof(MapEntry) + map->keySize;
  entry->hash = hash;
  memcpy(entry->key, key, map->keySize);
  memcpy(entry->value, value, map->valueSize);
  entry->next = map->buckets[index];
  map->buckets[index] = entry;
  map->size++;
  return true;
}

static bool MapGet(const Map* map, const void* key, void* outValue)
{
  if (!map || !key || !outValue) return false;
  const size_t hash = map->hashFn(key);
  const size_t index = hash & map->mask;
  const MapEntry* entry = map->buckets[index];
  while (entry)
  {
    if (entry->hash == hash && map->compareFn(entry->key, key))
    {
      memcpy(outValue, entry->value, map->valueSize);
      return true;
    }
    entry = entry->next;
  }
  return false;
}

static bool MapRemove(Map* map, const void* key)
{
  if (!map || !key) return false;
  const size_t hash = map->hashFn(key);
  const size_t index = hash & map->mask;
  MapEntry* entry = map->buckets[index];
  MapEntry* prev = NULL;
  while (entry)
  {
    if (entry->hash == hash && map->compareFn(entry->key, key))
    {
      if (prev)
        prev->next = entry->next;
      else
        map->buckets[index] = entry->next;
      free(entry);
      map->size--;
      return true;
    }
    prev = entry;
    entry = entry->next;
  }
  return false;
}

// Iterator interface for Map
typedef struct
{
  const Map* map;
  size_t bucketIndex;
  MapEntry* current;
} MapIterator;

static MapIterator MapIteratorCreate(const Map* map)
{
  MapIterator it = {map, 0, NULL};
  if (map)
  {
    for (size_t i = 0; i < map->capacity; i++)
    {
      if (map->buckets[i])
      {
        it.bucketIndex = i;
        it.current = map->buckets[i];
        break;
      }
    }
  }
  return it;
}

static bool MapIteratorNext(MapIterator* it, void* outKey, void* outValue)
{
  if (!it || !it->map || !it->current) return false;
  memcpy(outKey, it->current->key, it->map->keySize);
  memcpy(outValue, it->current->value, it->map->valueSize);
  if (it->current->next) { it->current = it->current->next; }
  else
  {
    it->current = NULL;
    for (size_t i = it->bucketIndex + 1; i < it->map->capacity; i++)
    {
      if (it->map->buckets[i])
      {
        it->bucketIndex = i;
        it->current = it->map->buckets[i];
        break;
      }
    }
  }
  return true;
}

static size_t MapSize(const Map* map) { return map ? map->size : 0; }

static size_t MapCapacity(const Map* map) { return map ? map->capacity : 0; }

// Example hash and compare functions

static size_t MapHashInt(const void* key)
{
  uint32_t x = *(const int*)key;
  x = (x >> 16 ^ x) * 0x45d9f3b;
  x = (x >> 16 ^ x) * 0x45d9f3b;
  x = x >> 16 ^ x;
  return x;
}

static size_t MapHashString(const void* key)
{
  const char* str = *(const char**)key;
  size_t hash = 5381;
  int c;
  while ((c = *str++))
    hash = (hash << 5) + hash + c;
  return hash;
}

static bool MapCompareInt(const void* key1, const void* key2)
{
  return *(const int*)key1 == *(const int*)key2;
}

static bool MapCompareString(const void* key1, const void* key2)
{
  return strcmp(*(const char**)key1, *(const char**)key2) == 0;
}

#endif // MAP_H
