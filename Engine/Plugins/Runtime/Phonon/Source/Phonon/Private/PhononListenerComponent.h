//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "Components/SceneComponent.h"
#include "PhononModule.h"

namespace Phonon
{
	class FPhononListenerObserver : public IAudioListenerObserver
	{
	public:
		FPhononListenerObserver();
		~FPhononListenerObserver();

		virtual void OnListenerUpdated(FAudioDevice* AudioDevice, UWorld* ListenerWorld, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds) override;
		virtual void OnListenerShutdown(FAudioDevice* AudioDevice) override;

	protected:
		bool bEnvironmentCreated;
		FPhononModule* PhononModule;
	};
}


