// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
AndroidAffinity.h: Android affinity profile masks definitions.
==============================================================================================*/

#pragma once

#include "GenericPlatformAffinity.h"

class FAndroidAffinity : public FGenericPlatformAffinity
{
public:
	static const CORE_API uint64 GetMainGameMask() OVERRIDE
	{
		return MAKEAFFINITYMASK1(0);
	}

	static const CORE_API uint64 GetRenderingThreadMask() OVERRIDE
	{
		return MAKEAFFINITYMASK1(1);
	}

	static const CORE_API uint64 GetRTHeartBeatMask() OVERRIDE
	{
		return MAKEAFFINITYMASK1(4);
	}

	static const CORE_API uint64 GetPoolThreadMask() OVERRIDE
	{
		return MAKEAFFINITYMASK1(4);
	}

	static const CORE_API uint64 GetTaskGraphThreadMask() OVERRIDE
	{
		return MAKEAFFINITYMASK3(0, 2, 3);
	}

	static const CORE_API uint64 GetStatsThreadMask() OVERRIDE
	{
		return MAKEAFFINITYMASK1(5);
	}
};

typedef FAndroidAffinity FPlatformAffinity;
