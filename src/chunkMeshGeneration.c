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
#include "utilities/map.h"
#include "utilities/darray.h"
#include "settings.h"

// Function Prototypes
static void GetVoxelsVertices(DArray* vertices, const Chunk* chunk);
static void AddFaceToMesh(DArray* vertices, Vector3I voxelPosition, Face face);
static void AddVoxelToMesh(DArray* vertices, const Chunk* chunk,
                           Vector3I voxelPosition);
static bool IsFaceExposed(const Chunk* chunk, Vector3I voxelPosition,
                          Face face);

void GenerateChunkMesh(Chunk* chunk)
{
  // Make sure we were given a real chunk
  if (chunk == NULL) return;

  // Ensures that previous mesh is unloaded
  if (chunk->mesh != NULL)
  {
    UnloadMesh(*chunk->mesh);
    free(chunk->mesh);
    chunk->mesh = NULL;
  }

  DArray* vertices = DArrayCreate(sizeof(Vector3I));

  // This will add all exposed voxels' vertices to the mesh INCLUDING DUPLICATES
  GetVoxelsVertices(vertices, chunk);

  // Make sure that there are vertices to generate a mesh
  if (DArraySize(vertices) == 0) goto SKIPMESHGEN;

  DArray* indices = DArrayCreate(sizeof(uint));
  Map* uniqueVertices = MapCreate(sizeof(Vector3I), sizeof(uint), MHVI, MCV3);

  // Adds unique vertices to the map
  for (size_t i = 0; i < DArraySize(vertices); i++)
  {
    Vector3I vertex;
    DArrayGet(vertices, i, &vertex);

    uint dummy;
    const bool found = MapGet(uniqueVertices, &vertex, &dummy);

    if (!found)
    {
      uint index = MapSize(uniqueVertices);
      MapPut(uniqueVertices, &vertex, &index);
    }
  }

  // Creates triangles from the vertices
  for (size_t i = 0; i < DArraySize(vertices); i += 4)
  {
    Vector3I index1, index2, index3, index4;
    DArrayGet(vertices, i, &index1);
    DArrayGet(vertices, i + 1, &index2);
    DArrayGet(vertices, i + 2, &index3);
    DArrayGet(vertices, i + 3, &index4);

    uint indice1, indice2, indice3, indice4;
    MapGet(uniqueVertices, &index1, &indice1);
    MapGet(uniqueVertices, &index2, &indice2);
    MapGet(uniqueVertices, &index3, &indice3);
    MapGet(uniqueVertices, &index4, &indice4);

    // First triangle CCW order
    DArrayPush(indices, &indice1);
    DArrayPush(indices, &indice2);
    DArrayPush(indices, &indice3);

    // Second triangle CCW order
    DArrayPush(indices, &indice1);
    DArrayPush(indices, &indice3);
    DArrayPush(indices, &indice4);
  }

  DArray* orderedVertices = DArrayCreate(sizeof(Vector3I));

  // Iterates through the map and adds all the unique vertices to the array, in
  // order
  MapIterator it = MapIteratorCreate(uniqueVertices);
  Vector3I vertex;
  uint index;
  while (MapIteratorNext(&it, &vertex, &index))
  {
    DArraySet(orderedVertices, index, &vertex);
  }

  // Create mesh from vertices and indices
  Mesh mesh = {0};
  mesh.vertexCount = (int)(MapSize(uniqueVertices) * 3);
  mesh.triangleCount = (int)(DArraySize(indices) / 3);
  mesh.vertices = (float*)malloc(DArraySize(vertices) * 3 * sizeof(float));
  mesh.indices = (ushort*)malloc(DArraySize(indices) * sizeof(ushort));

  // Adds vertices and indices to the mesh
  for (size_t i = 0; i < DArraySize(orderedVertices); i++)
  {
    Vector3I vertice;
    DArrayGet(orderedVertices, i, &vertice);

    mesh.vertices[i * 3] = (float)vertice.x;
    mesh.vertices[i * 3 + 1] = (float)vertice.y;
    mesh.vertices[i * 3 + 2] = (float)vertice.z;
  }

  for (size_t i = 0; i < DArraySize(indices); i++)
  {
    uint indice;
    DArrayGet(indices, i, &indice);
    mesh.indices[i] = indice;
  }

  // Sending the mesh to the GPU and adding it to the chunk
  UploadMesh(&mesh, false);
  chunk->mesh = (Mesh*)malloc(sizeof(Mesh));
  *chunk->mesh = mesh;

  // Cleanup
  DArrayFree(orderedVertices);
  DArrayFree(indices);
  MapFree(uniqueVertices);

SKIPMESHGEN:
  DArrayFree(vertices);
}

// Iterates through all voxels in a chunk and adds them to the mesh
static void GetVoxelsVertices(DArray* vertices, const Chunk* chunk)
{
  for (int x = 0; x < CHUNK_SIZE; x++)
  {
    for (int y = 0; y < CHUNK_SIZE; y++)
    {
      for (int z = 0; z < CHUNK_SIZE; z++)
      {
        if (chunk->voxels[x][y][z].type == AIR) continue;
        const Vector3I voxelPosition = {x, y, z};
        AddVoxelToMesh(vertices, chunk, voxelPosition);
      }
    }
  }
}

// Checks if a voxel is exposed and adds it to the mesh
static void AddVoxelToMesh(DArray* vertices, const Chunk* chunk,
                           const Vector3I voxelPosition)
{
  for (int face = 0; face < 6; face++)
  {
    if (IsFaceExposed(chunk, voxelPosition, face))
    {
      AddFaceToMesh(vertices, voxelPosition, face);
    }
  }
}

// Checks if a face is exposed
static bool IsFaceExposed(const Chunk* chunk, const Vector3I voxelPosition,
                          const Face face)
{
  Vector3I neighborPosition = voxelPosition;

  switch (face)
  {
    case TOP: neighborPosition.y++;
      break;
    case BOTTOM: neighborPosition.y--;
      break;
    case LEFT: neighborPosition.x--;
      break;
    case RIGHT: neighborPosition.x++;
      break;
    case FRONT: neighborPosition.z++;
      break;
    case BACK: neighborPosition.z--;
      break;
  }

  if (neighborPosition.x < 0 || neighborPosition.x >= CHUNK_SIZE) return true;
  if (neighborPosition.y < 0 || neighborPosition.y >= CHUNK_SIZE) return true;
  if (neighborPosition.z < 0 || neighborPosition.z >= CHUNK_SIZE) return true;

  const int neighborPosX = neighborPosition.x;
  const int neighborPosY = neighborPosition.y;
  const int neighborPosZ = neighborPosition.z;

  return chunk->voxels[neighborPosX][neighborPosY][neighborPosZ].type == AIR;
}

// Adds a face to the mesh
static void AddFaceToMesh(DArray* vertices, const Vector3I voxelPosition,
                          const Face face)
{
  // Static arrays to store face vertex offsets (formatter made this ugly)
  static const Vector3I topFace[4] = {{0, 1, 0},
                                      {0, 1, 1},
                                      {1, 1, 1},
                                      {1, 1, 0}};
  static const Vector3I bottomFace[4] = {{0, 0, 0},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {0, 0, 1}};
  static const Vector3I leftFace[4] = {{0, 0, 0},
                                       {0, 0, 1},
                                       {0, 1, 1},
                                       {0, 1, 0}};
  static const Vector3I rightFace[4] = {{1, 0, 0},
                                        {1, 1, 0},
                                        {1, 1, 1},
                                        {1, 0, 1}};
  static const Vector3I frontFace[4] = {{0, 0, 1},
                                        {1, 0, 1},
                                        {1, 1, 1},
                                        {0, 1, 1}};
  static const Vector3I backFace[4] = {{0, 0, 0},
                                       {0, 1, 0},
                                       {1, 1, 0},
                                       {1, 0, 0}};

  const Vector3I* faceVertices;
  switch (face)
  {
    case TOP: faceVertices = topFace;
      break;
    case BOTTOM: faceVertices = bottomFace;
      break;
    case LEFT: faceVertices = leftFace;
      break;
    case RIGHT: faceVertices = rightFace;
      break;
    case FRONT: faceVertices = frontFace;
      break;
    case BACK: faceVertices = backFace;
      break;
    default: faceVertices = frontFace;
      break;
  }

  for (int i = 0; i < 4; i++)
  {
    Vector3I vertex = {voxelPosition.x + faceVertices[i].x,
                       voxelPosition.y + faceVertices[i].y,
                       voxelPosition.z + faceVertices[i].z};

    DArrayPush(vertices, &vertex);
  }
}
