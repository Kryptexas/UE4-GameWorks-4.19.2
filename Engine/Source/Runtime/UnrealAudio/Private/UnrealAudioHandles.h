// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"
#include "UnrealAudioEntityManager.h"

#if ENABLE_UNREAL_AUDIO

namespace UAudio
{
	struct FSoundFileHandle : public FEntityHandle
	{
		FSoundFileHandle()
			: FEntityHandle()
		{}

		FSoundFileHandle(const FSoundFileHandle& InEntityHandle)
			: FEntityHandle(InEntityHandle)
		{}

		FSoundFileHandle(const FEntityHandle& InEntityHandle)
			: FEntityHandle(InEntityHandle)
		{}
	};

	struct FVoiceHandle : public FEntityHandle
	{
		FVoiceHandle()
			: FEntityHandle()
		{}

		FVoiceHandle(const FVoiceHandle& InEntityHandle)
			: FEntityHandle(InEntityHandle)
		{}

		FVoiceHandle(const FEntityHandle& InEntityHandle)
			: FEntityHandle(InEntityHandle)
		{}
	};

	struct FEmitterHandle : public FEntityHandle
	{
		FEmitterHandle()
			: FEntityHandle()
		{}

		FEmitterHandle(const FEntityHandle& InEntityHandle)
			: FEntityHandle(InEntityHandle)
		{}
	};

}

#endif // #if ENABLE_UNREAL_AUDIO


