// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericPlatform/GenericPlatformCriticalSection.h"
#include "PThreadCriticalSection.h"

typedef FPThreadsCriticalSection FCriticalSection;
typedef FSystemWideCriticalSectionNotImplemented FSystemWideCriticalSection;
