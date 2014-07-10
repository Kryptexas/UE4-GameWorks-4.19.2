// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Needed on platforms other than Windows
#ifndef WIN32
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#endif

#include <assert.h>
#define check(x)	assert(x)

// We can't use static_assert in .c files as this is a C++(11) feature
#if __cplusplus && !__clang__
#ifdef _MSC_VER
typedef unsigned long long uint64_t;
#endif // _MSC_VER
static_assert(sizeof(uint64_t) == 8, "Bad!");
#endif

#if __cplusplus
static inline bool isalpha(char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}
#endif
