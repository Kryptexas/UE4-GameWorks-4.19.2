// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Only needed on Mac
#if __APPLE__
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#endif

#include <assert.h>
#define check(x)	assert(x)

// On Mac we can't use static_assert in .c files as they really are compiled as C
#if !__APPLE__ || __cplusplus
typedef unsigned long long uint64_t;
static_assert(sizeof(uint64_t) == 8, "Bad!");
#endif

#if __cplusplus
static inline bool isalpha(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}
#endif
