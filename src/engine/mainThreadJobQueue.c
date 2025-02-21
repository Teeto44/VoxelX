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

#include "mainThreadJobQueue.h"
#include <stdlib.h>
#include "dataTypes.h"
#include "meshGenerationHelpers.h"
#include "raylib.h"
#include "voxelTasks.h"
#include "tinycthreadWrapper.h"

// Global job queue variables.
static MainThreadJob* jobQueueHead = NULL;
static MainThreadJob* jobQueueTail = NULL;
static mtx_t jobQueueMutex;

// Forward Declarations
static void ProcessMeshJob(MeshJob* meshJob);

void MainThreadJobQueueInit(void)
{
  mtx_init(&jobQueueMutex, mtx_plain);
  jobQueueHead = NULL;
  jobQueueTail = NULL;
}

void MainThreadJobQueueShutdown(void)
{
  mtx_lock(&jobQueueMutex);
  MainThreadJob* job = jobQueueHead;
  while (job)
  {
    MainThreadJob* next = job->next;
    free(job);
    job = next;
  }
  jobQueueHead = NULL;
  jobQueueTail = NULL;
  mtx_unlock(&jobQueueMutex);
  mtx_destroy(&jobQueueMutex);
}

void MainThreadJobQueuePushJob(MainThreadJob* job)
{
  if (!job) return;
  job->next = NULL;
  mtx_lock(&jobQueueMutex);
  if (jobQueueTail)
  {
    jobQueueTail->next = job;
    jobQueueTail = job;
  }
  else
  {
    jobQueueHead = job;
    jobQueueTail = job;
  }
  mtx_unlock(&jobQueueMutex);
}

void ProcessMainThreadJobs(void)
{
  while (1)
  {
    mtx_lock(&jobQueueMutex);
    MainThreadJob* job = jobQueueHead;
    if (job)
    {
      jobQueueHead = job->next;
      if (jobQueueHead == NULL) { jobQueueTail = NULL; }
    }
    mtx_unlock(&jobQueueMutex);
    if (!job) break;
    if (job->type == JOB_MESH_FINALIZE)
    {
      MeshJob* meshJob = (struct MeshJob*)job;
      ProcessMeshJob(meshJob);
    }
  }
}

static void ProcessMeshJob(MeshJob* meshJob)
{
  if (!meshJob || !meshJob->chunk) return;

  Chunk* chunk = meshJob->chunk;

  if (chunk->model.meshCount > 0) { RemoveChunkModel(chunk); }

  Mesh mesh = {0};
  mesh.vertexCount = meshJob->vertexCount;
  mesh.triangleCount = meshJob->vertexCount / 3;
  mesh.vertices = malloc(meshJob->vertexCount * 3 * sizeof(float));
  mesh.colors = malloc(meshJob->vertexCount * 4);
  if (!mesh.vertices || !mesh.colors)
  {
    TraceLog(LOG_ERROR,
             "Failed to allocate mesh data in main thread finalization");
    free(mesh.vertices);
    free(mesh.colors);
    free(meshJob->vertices);
    free(meshJob);
    return;
  }
  for (int i = 0; i < meshJob->vertexCount; i++)
  {
    const int vIdx = i * 3;
    const int cIdx = i * 4;
    mesh.vertices[vIdx] = meshJob->vertices[i].position.x;
    mesh.vertices[vIdx + 1] = meshJob->vertices[i].position.y;
    mesh.vertices[vIdx + 2] = meshJob->vertices[i].position.z;
    mesh.colors[cIdx] = meshJob->vertices[i].color.r;
    mesh.colors[cIdx + 1] = meshJob->vertices[i].color.g;
    mesh.colors[cIdx + 2] = meshJob->vertices[i].color.b;
    mesh.colors[cIdx + 3] = meshJob->vertices[i].color.a;
  }

  UploadMesh(&mesh, false);
  chunk->model = LoadModelFromMesh(mesh);
  chunk->needsMeshing = false;

  free(meshJob->vertices);
  free(meshJob);
}
