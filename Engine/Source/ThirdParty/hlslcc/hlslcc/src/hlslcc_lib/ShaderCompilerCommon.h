// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//#pragma once

#include <assert.h>
#define check(x)	assert(x)

typedef unsigned long long uint64_t;
static_assert(sizeof(uint64_t) == 8, "Bad!");
