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
*******************************************************************************/

#include "chunkMeshGeneration.h"
#include "chunkMap.h"
#include "darray.h"
#include "dataTypes.h"
#include "raylib.h"

// Function prototypes
static void GetVoxelsVertices(DArray* vertices, DArray* colors,
                              const Chunk* chunk);
static void AddVoxelToMesh(DArray* vertices, DArray* colors, const Chunk* chunk,
                           Vector3I voxelPosition);
static bool IsFaceExposed(const Chunk* chunk, Vector3I voxelPosition,
                          Face face);
static void AddFaceToMesh(DArray* vertices, DArray* colors,
                          Vector3I voxelPosition, Face face, Color color);
static Color GetVoxelColor(VoxelType type);
static VoxelType GetVoxelFromNeighboringChunk(const Chunk* chunk,
                                              Vector3I neighborPosition);

void GenerateChunkMesh(Chunk* chunk)
{
  // Make sure we were given a real chunk
  if (chunk == NULL)
  {
    TraceLog(LOG_ERROR, "Mesh generation received non-existent chunk");
    return;
  }

  // Ensures that previous mesh is unloaded
  if (chunk->model.meshCount > 0) { REMOVE_CHUNK_MODEL(chunk); }

  DArray* vertices = DArrayCreate(sizeof(Vector3I));
  DArray* colors = DArrayCreate(sizeof(Color));

  GetVoxelsVertices(vertices, colors, chunk);

  if (DArraySize(vertices) == 0) goto SKIPMESHGEN;

  DArrayShrinkToFit(vertices);
  DArrayShrinkToFit(colors);

  DArray* indices = DArrayCreate(sizeof(uint));

  for (size_t i = 0; i < DArraySize(vertices); i++)
  {
    DArrayPush(indices, &i);
  }

  DArrayShrinkToFit(indices);

  Mesh mesh = {0};
  mesh.vertexCount = (int)DArraySize(vertices);
  mesh.triangleCount = (int)DArraySize(indices) / 3;
  mesh.vertices = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));

  mesh.indices =
    (unsigned short*)malloc(DArraySize(indices) * sizeof(unsigned short));

  mesh.colors =
    (unsigned char*)malloc(DArraySize(colors) * 4 * sizeof(unsigned char));

  for (size_t i = 0; i < DArraySize(vertices); i++)
  {
    Vector3I vertice;
    DArrayGet(vertices, i, &vertice);
    mesh.vertices[i * 3] = (float)vertice.x;
    mesh.vertices[i * 3 + 1] = (float)vertice.y;
    mesh.vertices[i * 3 + 2] = (float)vertice.z;
  }

  for (size_t i = 0; i < DArraySize(indices); i++)
  {
    uint indice;
    DArrayGet(indices, i, &indice);
    mesh.indices[i] = (unsigned short)indice;
  }

  for (size_t i = 0; i < DArraySize(colors); i++)
  {
    Color color;
    DArrayGet(colors, i, &color);
    mesh.colors[i * 4] = color.r;
    mesh.colors[i * 4 + 1] = color.g;
    mesh.colors[i * 4 + 2] = color.b;
    mesh.colors[i * 4 + 3] = color.a;
  }

  UploadMesh(&mesh, false);
  chunk->model = LoadModelFromMesh(mesh);

  DArrayFree(indices);

SKIPMESHGEN:
  DArrayFree(vertices);
  DArrayFree(colors);
}

static void GetVoxelsVertices(DArray* vertices, DArray* colors,
                              const Chunk* chunk)
{
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      for (int z = 0; z < CHUNK_SIZE; z++)
      {
        if (chunk->voxels[x][y][z].type == AIR) continue;
        const Vector3I voxelPosition = {x, y, z};
        AddVoxelToMesh(vertices, colors, chunk, voxelPosition);
      }
    }
  }
}

static void AddVoxelToMesh(DArray* vertices, DArray* colors, const Chunk* chunk,
                           const Vector3I voxelPosition)
{
  const Color color = GetVoxelColor(
    chunk->voxels[voxelPosition.x][voxelPosition.y][voxelPosition.z].type);
  for (int face = 0; face < 6; face++)
  {
    if (IsFaceExposed(chunk, voxelPosition, face))
    {
      AddFaceToMesh(vertices, colors, voxelPosition, face, color);
    }
  }
}

static bool IsFaceExposed(const Chunk* chunk, const Vector3I voxelPosition,
                          const Face face)
{
  int neighborPosX = voxelPosition.x;
  int neighborPosY = voxelPosition.y;
  int neighborPosZ = voxelPosition.z;
  switch (face)
  {
    case TOP: neighborPosY++; break;
    case BOTTOM: neighborPosY--; break;
    case LEFT: neighborPosX--; break;
    case RIGHT: neighborPosX++; break;
    case FRONT: neighborPosZ++; break;
    case BACK: neighborPosZ--; break;
  }

  // Check if the neighbor position is within the current chunk
  if (neighborPosX >= 0 && neighborPosX < CHUNK_SIZE && neighborPosY >= 0 &&
      neighborPosY < CHUNK_SIZE && neighborPosZ >= 0 &&
      neighborPosZ < CHUNK_SIZE)
  {
    return chunk->voxels[neighborPosX][neighborPosY][neighborPosZ].type == AIR;
  }

  // If the neighbor position is outside the current chunk
  return GetVoxelFromNeighboringChunk(
           chunk, (Vector3I){neighborPosX, neighborPosY, neighborPosZ}) == AIR;
}

static void AddFaceToMesh(DArray* vertices, DArray* colors,
                          const Vector3I voxelPosition, const Face face,
                          const Color color)
{
  static const Vector3I topFace[6] = {{0, 1, 0}, {0, 1, 1}, {1, 1, 1},
                                      {0, 1, 0}, {1, 1, 1}, {1, 1, 0}};
  static const Vector3I bottomFace[6] = {{0, 0, 0}, {1, 0, 0}, {1, 0, 1},
                                         {0, 0, 0}, {1, 0, 1}, {0, 0, 1}};
  static const Vector3I leftFace[6] = {{0, 0, 0}, {0, 0, 1}, {0, 1, 1},
                                       {0, 0, 0}, {0, 1, 1}, {0, 1, 0}};
  static const Vector3I rightFace[6] = {{1, 0, 0}, {1, 1, 0}, {1, 1, 1},
                                        {1, 0, 0}, {1, 1, 1}, {1, 0, 1}};
  static const Vector3I frontFace[6] = {{0, 0, 1}, {1, 0, 1}, {1, 1, 1},
                                        {0, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  static const Vector3I backFace[6] = {{0, 0, 0}, {0, 1, 0}, {1, 1, 0},
                                       {0, 0, 0}, {1, 1, 0}, {1, 0, 0}};
  const Vector3I* faceVertices;
  Color shadedColor = color;
  switch (face)
  {
    case TOP:
      faceVertices = topFace;
      shadedColor = (Color){(unsigned char)((float)color.r * 1.0f),
                            (unsigned char)((float)color.g * 1.0f),
                            (unsigned char)((float)color.b * 1.0f), color.a};
      break;
    case BOTTOM:
      faceVertices = bottomFace;
      shadedColor = (Color){(unsigned char)((float)color.r * 0.5f),
                            (unsigned char)((float)color.g * 0.5f),
                            (unsigned char)((float)color.b * 0.5f), color.a};
      break;
    case LEFT:
      faceVertices = leftFace;
      shadedColor = (Color){(unsigned char)((float)color.r * 0.75f),
                            (unsigned char)((float)color.g * 0.7f),
                            (unsigned char)((float)color.b * 0.7f), color.a};
      break;
    case RIGHT:
      faceVertices = rightFace;
      shadedColor = (Color){(unsigned char)((float)color.r * 0.75f),
                            (unsigned char)((float)color.g * 0.75f),
                            (unsigned char)((float)color.b * 0.75f), color.a};
      break;
    case FRONT:
      faceVertices = frontFace;
      shadedColor = (Color){(unsigned char)((float)color.r * 0.75f),
                            (unsigned char)((float)color.g * 0.75f),
                            (unsigned char)((float)color.b * 0.75f), color.a};
      break;
    case BACK:
      faceVertices = backFace;
      shadedColor = (Color){(unsigned char)((float)color.r * 0.75f),
                            (unsigned char)((float)color.g * 0.75f),
                            (unsigned char)((float)color.b * 0.75f), color.a};
      break;
    default: faceVertices = topFace; break;
  }

  for (int i = 0; i < 6; i++)
  {
    Vector3I vertex = {voxelPosition.x + faceVertices[i].x,
                       voxelPosition.y + faceVertices[i].y,
                       voxelPosition.z + faceVertices[i].z};
    DArrayPush(vertices, &vertex);
    DArrayPush(colors, &shadedColor);
  }
}

static Color GetVoxelColor(const VoxelType type)
{
  switch (type)
  {
    case STONE: return (Color){128, 128, 128, 255};
    case DIRT: return (Color){139, 69, 19, 255};
    case GRASS: return (Color){34, 139, 34, 255};
    case AIR: return (Color){0, 0, 0, 0};
    default: return (Color){255, 255, 255, 255};
  }
}

static VoxelType GetVoxelFromNeighboringChunk(const Chunk* chunk,
                                              Vector3I neighborPosition)
{
  if (loadedChunks == NULL)
  {
    TraceLog(LOG_ERROR, "Mesh generation cannot access chunks");
    return AIR;
  }

  int chunkX = chunk->position.x;
  int chunkY = chunk->position.y;
  int chunkZ = chunk->position.z;

  if (neighborPosition.x < 0)
  {
    chunkX--;
    neighborPosition.x += CHUNK_SIZE;
  }
  else if (neighborPosition.x >= CHUNK_SIZE)
  {
    chunkX++;
    neighborPosition.x -= CHUNK_SIZE;
  }

  if (neighborPosition.y < 0)
  {
    chunkY--;
    neighborPosition.y += CHUNK_SIZE;
  }
  else if (neighborPosition.y >= CHUNK_SIZE)
  {
    chunkY++;
    neighborPosition.y -= CHUNK_SIZE;
  }

  if (neighborPosition.z < 0)
  {
    chunkZ--;
    neighborPosition.z += CHUNK_SIZE;
  }
  else if (neighborPosition.z >= CHUNK_SIZE)
  {
    chunkZ++;
    neighborPosition.z -= CHUNK_SIZE;
  }

  const Chunk* neighborChunk = GetChunkFromMap(chunkX, chunkY, chunkZ);
  if (neighborChunk == NULL) return AIR;

  return neighborChunk
    ->voxels[neighborPosition.x][neighborPosition.y][neighborPosition.z]
    .type;
}
