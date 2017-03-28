//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononListenerComponent.h"
#include "PhononModule.h"
#include "PhononOcclusion.h"
#include "PhononReverb.h"

namespace Phonon
{
	FPhononListenerObserver::FPhononListenerObserver()
		: bEnvironmentCreated(false)
		, PhononModule(nullptr)
	{

	}

	FPhononListenerObserver::~FPhononListenerObserver()
	{

	}

	void FPhononListenerObserver::OnListenerUpdated(FAudioDevice* AudioDevice, UWorld* ListenerWorld, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds)
	{
		// TODO: SUPPORT MULTIPLE WORLDS
		if (!bEnvironmentCreated)
		{
			bEnvironmentCreated = true;
			PhononModule = &FModuleManager::GetModuleChecked<FPhononModule>("Phonon");
			PhononModule->CreateEnvironment(ListenerWorld, AudioDevice);
		}

		check(PhononModule);

		FVector Position = ListenerTransform.GetLocation();
		FVector Forward = ListenerTransform.GetUnitAxis(EAxis::Y);
		FVector Up = ListenerTransform.GetUnitAxis(EAxis::Z);

		Phonon::FPhononOcclusion* OcclusionInstance = PhononModule->GetOcclusionInstance();
		if (OcclusionInstance)
		{
			OcclusionInstance->UpdateDirectSoundSources(Position, Forward, Up);
		}

		Phonon::FPhononReverb* ReverbInstance = PhononModule->GetReverbInstance();
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
			PhononModule->DestroyEnvironment(AudioDevice);
			PhononModule = nullptr;
		}
	}
}
