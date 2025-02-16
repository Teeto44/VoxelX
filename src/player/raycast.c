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

#include "raycast.h"
#include "raymath.h"
#include "world.h"

RaycastResult Raycast(const Vector3 start, const Vector3 direction,
                      const float distance)
{
  const Vector3 rayDir = Vector3Normalize(direction);
  Vector3 currentPos = start;
  const Vector3 stepDir = {rayDir.x >= 0 ? 1 : -1, rayDir.y >= 0 ? 1 : -1,
                           rayDir.z >= 0 ? 1 : -1};

  // Distance from one grid line to next for each axis
  const Vector3 delta = {fabsf(1.0f / rayDir.x), fabsf(1.0f / rayDir.y),
                         fabsf(1.0f / rayDir.z)};

  // Distance to first grid line for each axis
  Vector3 dist = {stepDir.x > 0 ? (ceilf(start.x) - start.x) * delta.x
                                : (start.x - floorf(start.x)) * delta.x,
                  stepDir.y > 0 ? (ceilf(start.y) - start.y) * delta.y
                                : (start.y - floorf(start.y)) * delta.y,
                  stepDir.z > 0 ? (ceilf(start.z) - start.z) * delta.z
                                : (start.z - floorf(start.z)) * delta.z};

  float totalDist = 0.0f;
  int lastAxis; // Track which axis we moved along

  while (totalDist < distance)
  {
    // Find axis with the shortest path
    if (dist.x < dist.y && dist.x < dist.z)
    {
      totalDist = dist.x;
      currentPos.x += stepDir.x;
      dist.x += delta.x;
      lastAxis = 0;
    }
    else if (dist.y < dist.z)
    {
      totalDist = dist.y;
      currentPos.y += stepDir.y;
      dist.y += delta.y;
      lastAxis = 1;
    }
    else
    {
      totalDist = dist.z;
      currentPos.z += stepDir.z;
      dist.z += delta.z;
      lastAxis = 2;
    }

    // Check for collision
    const Voxel voxel = GetVoxel(currentPos);
    if (voxel.type != AIR)
    {
      Vector3 normal = {0};
      // Set normal based on the axis we moved along
      switch (lastAxis)
      {
        default: normal.x = -stepDir.x; break;
        case 1: normal.y = -stepDir.y; break;
        case 2: normal.z = -stepDir.z; break;
      }

      return (RaycastResult){true, currentPos, voxel, normal};
    }
  }

  return (RaycastResult){0};
}
