#pragma once

#ifdef WIN32

#else
typedef int SOCKET;
#define SOCKET_ERROR	(-1)
#define INVALID_SOCKET  (-1)
#endif
