#pragma once

#ifdef WIN32
#include <windows.h>
#else
#endif

int getThreadID() {
    int id = 0;
#ifdef WIN32
    id = GetCurrentThreadId();
#else
    id = getpid();
#endif
    return id;
}