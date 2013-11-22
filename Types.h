#pragma once

typedef signed char		i8;
typedef short			i16;
typedef int			i32;
#if defined(_WIN32)
typedef __int64			i64;
#else
typedef long			i64;
#endif

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned		u32;
#if defined(_WIN32)
typedef __int64			u64;
#else
typedef unsigned long		u64;
#endif

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

#if defined(_WIN32)
#define Strcasecmp stricmp
#else
#define Strcasecmp strcasecmp
#endif