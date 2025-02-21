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

#include "voxelTasks.h"
#include <stdlib.h>
#include "mainThreadJobQueue.h"
#include "meshGenerationHelpers.h"
#include "threadpool.h"
#include "worldGeneration.h"

static MeshJob* ComputeChunkMesh(Chunk* chunk)
{
  if (!chunk->voxels)
  {
    chunk->needsMeshing = false;
    return NULL;
  }

  int vertexCount = 0;
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      for (int z = 0; z < CHUNK_SIZE; z++)
      {
        const VoxelType type = chunk->voxels[VOXEL_INDEX(x, y, z)].type;
        if (type == AIR) continue;
        for (Face face = 0; face < 6; face++)
        {
          if (IsFaceExposed(chunk, x, y, z, face)) { vertexCount += 6; }
        }
      }
    }
  }

  if (vertexCount == 0)
  {
    chunk->needsMeshing = false;
    return NULL;
  }

  MeshJob* job = malloc(sizeof(MeshJob));
  if (!job)
  {
    TraceLog(LOG_ERROR, "Failed to allocate MeshJob");
    return NULL;
  }
  job->base.type = JOB_MESH_FINALIZE;
  job->chunk = chunk;
  job->vertexCount = vertexCount;
  job->vertices = malloc(vertexCount * sizeof(Vertex));
  if (!job->vertices)
  {
    TraceLog(LOG_ERROR, "Failed to allocate vertex data for MeshJob");
    free(job);
    return NULL;
  }

  int currentVertex = 0;
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      for (int z = 0; z < CHUNK_SIZE; z++)
      {
        const VoxelType type = chunk->voxels[VOXEL_INDEX(x, y, z)].type;
        if (type == AIR) continue;
        Color baseColor;
        if (type < sizeof(voxelColors) / sizeof(voxelColors[0]))
          baseColor = voxelColors[type];
        else
          baseColor = WHITE;
        for (Face face = 0; face < 6; face++)
        {
          if (!IsFaceExposed(chunk, x, y, z, face)) continue;
          const Color shadedColor =
            ApplyShading(baseColor, faces[face].shadeFactor);
          const float floatX = (float)x;
          const float floatY = (float)y;
          const float floatZ = (float)z;
          for (int v = 0; v < 6; v++)
          {
            job->vertices[currentVertex].position.x =
              floatX + faces[face].vertices[v].x;
            job->vertices[currentVertex].position.y =
              floatY + faces[face].vertices[v].y;
            job->vertices[currentVertex].position.z =
              floatZ + faces[face].vertices[v].z;
            job->vertices[currentVertex].color = shadedColor;
            currentVertex++;
          }
        }
      }
    }
  }

  return job;
}

void ChunkGenerationTask(void* arg)
{
  if (!arg) return;
  Chunk* chunk = arg;
  GenerateChunk(chunk);

  extern ThreadPool threadPool;
  ThreadPoolSubmit(&threadPool, ComputeMeshTask, chunk);
}

void ComputeMeshTask(void* arg)
{
  if (!arg) return;
  Chunk* chunk = arg;
  MeshJob* job = ComputeChunkMesh(chunk);
  if (job) { MainThreadJobQueuePushJob((MainThreadJob*)job); }
}
