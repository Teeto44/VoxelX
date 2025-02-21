// This is not a tinycthread file, this is a file created for VoxelX project
// due to issues with raylib and windows.h

#ifndef TINYCTHREAD_WRAPPER_H
#define TINYCTHREAD_WRAPPER_H

// This avoids windows conflicting with raylib
#if defined(_WIN32)
  #define NOGDI
  #define NOUSER
#endif

#include "tinycthread.h"

#if defined(_WIN32)
  #undef near
  #undef far
#endif

#endif // TINYCTHREAD_WRAPPER_H