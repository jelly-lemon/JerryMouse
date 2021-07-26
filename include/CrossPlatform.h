#pragma once

/**
 * 跨平台时为了编写代码，统一命名一些 Windows 和 Linux 不同名字但含义相同的类型
 */

#ifdef WIN32

#else
typedef int SOCKET;
#define SOCKET_ERROR	(-1)
#define INVALID_SOCKET  (-1)
#endif
