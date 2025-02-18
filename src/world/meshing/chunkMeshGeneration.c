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

#include "chunkMeshGeneration.h"
#include <stdlib.h>
#include "chunkMap.h"
#include "raylib.h"

typedef struct
{
  Vector3 position;
  Color color;
} Vertex;

typedef struct
{
  Vector3 vertices[6];
  float shadeFactor;
} FaceData;

static const FaceData faces[6] = {
  // TOP (+Y)
  {{{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {0, 1, 0}, {1, 1, 1}, {1, 1, 0}}, 1.0f},
  // BOTTOM (-Y)
  {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 0}, {1, 0, 1}, {0, 0, 1}}, 0.5f},
  // LEFT (-X)
  {{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 0, 0}, {0, 1, 1}, {0, 1, 0}}, 0.7f},
  // RIGHT (+X)
  {{{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 0}, {1, 1, 1}, {1, 0, 1}}, 0.75f},
  // FRONT (+Z)
  {{{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 0, 1}, {1, 1, 1}, {0, 1, 1}}, 0.75f},
  // BACK (-Z)
  {{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 0}, {1, 1, 0}, {1, 0, 0}}, 0.75f}};

static const Color voxelColors[] = {
  {0, 0, 0, 0},        // AIR = 0
  {150, 75, 0, 255},   // DIRT = 1
  {46, 125, 50, 255},  // GRASS = 2
  {100, 100, 100, 255} // STONE = 3
};

static Color ApplyShading(const Color base, const float factor)
{
  return (Color){(unsigned char)((float)base.r * factor),
                 (unsigned char)((float)base.g * factor),
                 (unsigned char)((float)base.b * factor), base.a};
}

static bool IsFaceExposed(const Chunk* chunk, const int x, const int y,
                          const int z, const Face face)
{
  int neighborX = x + (face == RIGHT) - (face == LEFT);
  int neighborY = y + (face == TOP) - (face == BOTTOM);
  int neighborZ = z + (face == FRONT) - (face == BACK);

  // Check within current chunk bounds
  if (neighborX >= 0 && neighborX < CHUNK_SIZE && neighborY >= 0 &&
      neighborY < CHUNK_SIZE && neighborZ >= 0 && neighborZ < CHUNK_SIZE)
  {
    // If the current chunk’s voxel data is missing, it's all air
    return chunk->voxels[VOXEL_INDEX(neighborX, neighborY, neighborZ)].type ==
           AIR;
  }

  // Calculate neighbor chunk coordinates
  const int chunkOffsetX = neighborX < 0 ? -1 : neighborX >= CHUNK_SIZE ? 1 : 0;
  const int chunkOffsetY = neighborY < 0 ? -1 : neighborY >= CHUNK_SIZE ? 1 : 0;
  const int chunkOffsetZ = neighborZ < 0 ? -1 : neighborZ >= CHUNK_SIZE ? 1 : 0;

  const Chunk* neighbor = GetChunkFromMap(chunk->position.x + chunkOffsetX,
                                          chunk->position.y + chunkOffsetY,
                                          chunk->position.z + chunkOffsetZ);

  if (!neighbor) return true;

  // Wrap coordinates to neighbor chunk space
  neighborX = (neighborX + CHUNK_SIZE) % CHUNK_SIZE;
  neighborY = (neighborY + CHUNK_SIZE) % CHUNK_SIZE;
  neighborZ = (neighborZ + CHUNK_SIZE) % CHUNK_SIZE;

  if (!neighbor->voxels) return true;
  return neighbor->voxels[VOXEL_INDEX(neighborX, neighborY, neighborZ)].type ==
         AIR;
}

void GenerateChunkMesh(Chunk* chunk)
{
  if (!chunk)
  {
    TraceLog(LOG_ERROR, "Null chunk passed to mesh generation");
    return;
  }

  // If no voxel data is allocated, the chunk is entirely AIR
  if (!chunk->voxels)
  {
    chunk->needsMeshing = false;
    return;
  }

  // Skip if mesh is already generated and chunk hasn't changed
  if (chunk->model.meshCount > 0 && !chunk->needsMeshing) { return; }

  if (chunk->model.meshCount > 0) { REMOVE_CHUNK_MODEL(chunk); }

  // Count vertices first to avoid over-allocation
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
    return;
  }

  // Allocate exact size needed
  Vertex* vertices = malloc(vertexCount * sizeof(Vertex));
  int currentVertex = 0;

  // Generate mesh data
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      for (int z = 0; z < CHUNK_SIZE; z++)
      {
        const VoxelType type = chunk->voxels[VOXEL_INDEX(x, y, z)].type;
        if (type == AIR) continue;

        const Color baseColor = voxelColors[type];

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
            vertices[currentVertex].position.x =
              floatX + faces[face].vertices[v].x;
            vertices[currentVertex].position.y =
              floatY + faces[face].vertices[v].y;
            vertices[currentVertex].position.z =
              floatZ + faces[face].vertices[v].z;
            vertices[currentVertex].color = shadedColor;
            currentVertex++;
          }
        }
      }
    }
  }

  // Create and upload mesh
  Mesh mesh = {0};
  mesh.vertexCount = vertexCount;
  mesh.triangleCount = vertexCount / 3;
  mesh.vertices = malloc(vertexCount * 3 * sizeof(float));
  mesh.colors = malloc(vertexCount * 4);

  for (int i = 0; i < vertexCount; i++)
  {
    const int vIdx = i * 3;
    const int cIdx = i * 4;
    mesh.vertices[vIdx] = vertices[i].position.x;
    mesh.vertices[vIdx + 1] = vertices[i].position.y;
    mesh.vertices[vIdx + 2] = vertices[i].position.z;
    mesh.colors[cIdx] = vertices[i].color.r;
    mesh.colors[cIdx + 1] = vertices[i].color.g;
    mesh.colors[cIdx + 2] = vertices[i].color.b;
    mesh.colors[cIdx + 3] = vertices[i].color.a;
  }

  UploadMesh(&mesh, false);
  chunk->model = LoadModelFromMesh(mesh);
  chunk->needsMeshing = false;

  free(vertices);
}
