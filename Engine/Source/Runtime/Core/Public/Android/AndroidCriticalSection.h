// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PThreadCriticalSection.h"
#include "GenericPlatform/GenericPlatformCriticalSection.h"
#include "PThreadRWLock.h"

typedef FPThreadsCriticalSection FCriticalSection;
typedef FSystemWideCriticalSectionNotImplemented FSystemWideCriticalSection;
typedef FPThreadsRWLock FRWLock;
