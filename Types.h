#pragma once

typedef signed char		i8;
typedef short			i16;
typedef int			i32;
typedef long			i64;

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned		u32;
typedef unsigned long		u64;

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

