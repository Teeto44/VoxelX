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

#include "threadpool.h"
#include <stdlib.h>
#include "raylib.h"

static int ThreadPoolWorker(void* arg)
{
  ThreadPool* pool = arg;
  while (1)
  {
    mtx_lock(&pool->queueMutex);
    while (!pool->taskQueueHead && !pool->shutdown)
    {
      cnd_wait(&pool->queueCond, &pool->queueMutex);
    }
    if (pool->shutdown && !pool->taskQueueHead)
    {
      mtx_unlock(&pool->queueMutex);
      break;
    }
    ThreadPoolTask* task = pool->taskQueueHead;
    if (task)
    {
      pool->taskQueueHead = task->next;
      if (!pool->taskQueueHead) { pool->taskQueueTail = NULL; }
    }
    mtx_unlock(&pool->queueMutex);
    if (task)
    {
      task->function(task->arg);
      free(task);
    }
  }
  return 0;
}

bool ThreadPoolInit(ThreadPool* pool, const int numThreads)
{
  if (!pool || numThreads <= 0) return false;
  pool->numThreads = numThreads;
  pool->shutdown = false;
  pool->taskQueueHead = NULL;
  pool->taskQueueTail = NULL;

  if (mtx_init(&pool->queueMutex, mtx_plain) != thrd_success)
  {
    TraceLog(LOG_ERROR, "ThreadPoolInit: mtx_init failed");
    return false;
  }

  if (cnd_init(&pool->queueCond) != thrd_success)
  {
    TraceLog(LOG_ERROR, "ThreadPoolInit: cnd_init failed");
    mtx_destroy(&pool->queueMutex);
    return false;
  }

  pool->threads = malloc(sizeof(thrd_t) * numThreads);
  if (!pool->threads)
  {
    TraceLog(LOG_ERROR, "ThreadPoolInit: malloc for threads failed");
    cnd_destroy(&pool->queueCond);
    mtx_destroy(&pool->queueMutex);
    return false;
  }

  for (int i = 0; i < numThreads; i++)
  {
    if (thrd_create(&pool->threads[i], ThreadPoolWorker, pool) != thrd_success)
    {
      TraceLog(LOG_ERROR, "ThreadPoolInit: thrd_create failed at thread %d", i);
      pool->shutdown = true;
      cnd_broadcast(&pool->queueCond);
      for (int j = 0; j < i; j++)
      {
        thrd_join(pool->threads[j], NULL);
      }
      free(pool->threads);
      cnd_destroy(&pool->queueCond);
      mtx_destroy(&pool->queueMutex);
      return false;
    }
  }

  TraceLog(LOG_INFO, "Thread pool initialized with %d threads", numThreads);
  return true;
}

bool ThreadPoolSubmit(ThreadPool* pool, void (*function)(void*), void* arg)
{
  if (!pool || pool->shutdown) return false;
  ThreadPoolTask* task = malloc(sizeof(ThreadPoolTask));
  if (!task) return false;
  task->function = function;
  task->arg = arg;
  task->next = NULL;

  mtx_lock(&pool->queueMutex);
  if (pool->taskQueueTail)
  {
    pool->taskQueueTail->next = task;
    pool->taskQueueTail = task;
  }
  else
  {
    pool->taskQueueHead = task;
    pool->taskQueueTail = task;
  }
  cnd_signal(&pool->queueCond);
  mtx_unlock(&pool->queueMutex);
  return true;
}

void ThreadPoolShutdown(ThreadPool* pool)
{
  if (!pool) return;
  mtx_lock(&pool->queueMutex);
  pool->shutdown = true;
  cnd_broadcast(&pool->queueCond);
  mtx_unlock(&pool->queueMutex);
  for (int i = 0; i < pool->numThreads; i++)
  {
    thrd_join(pool->threads[i], NULL);
  }
  free(pool->threads);
  while (pool->taskQueueHead)
  {
    ThreadPoolTask* task = pool->taskQueueHead;
    pool->taskQueueHead = task->next;
    free(task);
  }
  cnd_destroy(&pool->queueCond);
  mtx_destroy(&pool->queueMutex);
}
