// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceModule.h"
#include "UnrealAudioTests.h"

#if ENABLE_UNREAL_AUDIO

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealAudio, Log, All);

namespace UAudio
{
	class FUnrealAudioModule :	public IUnrealAudioModule
	{
	public:

		FUnrealAudioModule();
		~FUnrealAudioModule();

		void Initialize() override;
		void Shutdown() override;

		class IUnrealAudioDeviceModule* GetDeviceModule();

		FName GetDeviceModuleName() const;

	private:

		static void InitializeTests(FUnrealAudioModule* Module);

		class IUnrealAudioDeviceModule* UnrealAudioDevice;
	};
}

#endif // #if ENABLE_UNREAL_AUDIO


