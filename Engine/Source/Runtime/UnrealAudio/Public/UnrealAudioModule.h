// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "ModuleInterface.h"

// Only enable unreal audio on windows
#if PLATFORM_WINDOWS
#define ENABLE_UNREAL_AUDIO 1
#else
#define ENABLE_UNREAL_AUDIO 0
#endif

namespace UAudio
{
	class UNREALAUDIO_API IUnrealAudioModule :  public IModuleInterface
	{
	public:
		virtual ~IUnrealAudioModule() {}

		virtual void Initialize()
		{
		}

		virtual void Shutdown()
		{
		}
	};
}



