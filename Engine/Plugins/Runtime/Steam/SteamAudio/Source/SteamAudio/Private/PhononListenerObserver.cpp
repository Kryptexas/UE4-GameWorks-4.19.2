//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononListenerObserver.h"
#include "SteamAudioModule.h"
#include "PhononOcclusion.h"
#include "PhononReverb.h"
#include "AudioDevice.h"

namespace SteamAudio
{
	FPhononListenerObserver::FPhononListenerObserver()
		: bEnvironmentCreated(false)
		, SteamAudioModule(nullptr)
	{
	}

	FPhononListenerObserver::~FPhononListenerObserver()
	{
	}

	void FPhononListenerObserver::OnListenerUpdated(FAudioDevice* AudioDevice, UWorld* ListenerWorld, const int32 ViewportIndex, 
		const FTransform& ListenerTransform, const float InDeltaSeconds)
	{
		// TODO: SUPPORT MULTIPLE WORLDS
#if WITH_EDITOR
		if (AudioDevice->IsMainAudioDevice())
		{
			return;
		}
#endif

		if (!bEnvironmentCreated)
		{
			bEnvironmentCreated = true;
			SteamAudioModule = &FModuleManager::GetModuleChecked<FSteamAudioModule>("SteamAudio");
			SteamAudioModule->CreateEnvironment(ListenerWorld, AudioDevice);
		}

		check(SteamAudioModule);

		FVector Position = ListenerTransform.GetLocation();
		FVector Forward = ListenerTransform.GetUnitAxis(EAxis::Y);
		FVector Up = ListenerTransform.GetUnitAxis(EAxis::Z);

		SteamAudio::FPhononOcclusion* OcclusionInstance = SteamAudioModule->GetOcclusionInstance();
		if (OcclusionInstance)
		{
			OcclusionInstance->UpdateDirectSoundSources(Position, Forward, Up);
		}

		SteamAudio::FPhononReverb* ReverbInstance = SteamAudioModule->GetReverbInstance();
		if (ReverbInstance)
		{
			ReverbInstance->UpdateListener(Position, Forward, Up);
		}
	}

	void FPhononListenerObserver::OnListenerShutdown(FAudioDevice* AudioDevice)
	{
		if (bEnvironmentCreated)
		{
			bEnvironmentCreated = false;
			SteamAudioModule->DestroyEnvironment(AudioDevice);
			SteamAudioModule = nullptr;
		}
	}
}
