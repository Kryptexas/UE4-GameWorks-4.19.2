// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"

// For some reason this isn't getting set
#define __WINDOWS_DS__

// And for some reason this is still getting set...
#ifdef __WINDOWS_WASAPI__
#undef __WINDOWS_WASAPI__
#endif