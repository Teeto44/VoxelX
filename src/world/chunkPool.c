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

#include "chunkPool.h"
#include <stdlib.h>

#define CHUNK_POOL_BLOCK_SIZE 64

typedef struct ChunkPoolBlock
{
  Chunk* chunks;               // contiguous block of chunks
  int usageCount;              // number of chunks in use
  struct ChunkPoolBlock* next; // linked list of blocks
} ChunkPoolBlock;

static ChunkPoolBlock* poolBlocks = NULL;
static Chunk* freeList = NULL;

void ChunkPoolInit(void)
{
  poolBlocks = NULL;
  freeList = NULL;
}

// Allocate a new block of chunks and add them to the free list
static void ChunkPoolAllocateBlock(void)
{
  ChunkPoolBlock* block = malloc(sizeof(ChunkPoolBlock));
  if (!block) { return; }

  block->chunks = malloc(CHUNK_POOL_BLOCK_SIZE * sizeof(Chunk));
  if (!block->chunks)
  {
    free(block);
    return;
  }
  block->usageCount = 0;
  for (int i = 0; i < CHUNK_POOL_BLOCK_SIZE; i++)
  {
    block->chunks[i].block = block;
    block->chunks[i].voxels = NULL;
    block->chunks[i].needsMeshing = false;
    block->chunks[i].model = (Model){0};
    block->chunks[i].nextFree = freeList;
    freeList = &block->chunks[i];
  }
  block->next = poolBlocks;
  poolBlocks = block;
}

Chunk* ChunkPoolAcquire(void)
{
  if (!freeList)
  {
    ChunkPoolAllocateBlock();
    if (!freeList) return NULL; // allocation failure
  }
  Chunk* chunk = freeList;
  freeList = chunk->nextFree;
  chunk->nextFree = NULL;
  if (chunk->block) chunk->block->usageCount++;
  return chunk;
}

// Remove from freeList all chunks belonging to a given block
static void RemoveBlockFromFreeList(const ChunkPoolBlock* block)
{
  Chunk** current = &freeList;
  while (*current)
  {
    if ((*current)->block == block) { *current = (*current)->nextFree; }
    else { current = &(*current)->nextFree; }
  }
}

// If a block has zero usage, free it completely
static void FreeBlock(ChunkPoolBlock* block)
{
  RemoveBlockFromFreeList(block);
  ChunkPoolBlock** current = &poolBlocks;
  while (*current)
  {
    if (*current == block)
    {
      *current = block->next;
      break;
    }
    current = &(*current)->next;
  }
  free(block->chunks);
  free(block);
}

void ChunkPoolRelease(Chunk* chunk)
{
  if (!chunk) return;
  ChunkPoolBlock* block = chunk->block;
  chunk->position = (Vector3I){0};
  if (chunk->voxels)
  {
    free(chunk->voxels);
    chunk->voxels = NULL;
  }
  if (chunk->model.meshCount > 0)
  {
    UnloadModel(chunk->model);
    chunk->model.meshCount = 0;
  }
  chunk->nextFree = freeList;
  freeList = chunk;
  if (block)
  {
    block->usageCount--;
    if (block->usageCount == 0) FreeBlock(block);
  }
}

void ChunkPoolShutdown(void)
{
  ChunkPoolBlock* block = poolBlocks;
  while (block)
  {
    ChunkPoolBlock* next = block->next;
    free(block->chunks);
    free(block);
    block = next;
  }
  poolBlocks = NULL;
  freeList = NULL;
}
