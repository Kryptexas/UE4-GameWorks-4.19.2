//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "SteamAudioModule.h"

namespace SteamAudio
{
	class FPhononListenerObserver : public IAudioListenerObserver
	{
	public:
		FPhononListenerObserver();
		~FPhononListenerObserver();

		virtual void OnListenerUpdated(FAudioDevice* AudioDevice, UWorld* ListenerWorld, const int32 ViewportIndex, 
			const FTransform& ListenerTransform, const float InDeltaSeconds) override;
		virtual void OnListenerShutdown(FAudioDevice* AudioDevice) override;

	protected:
		bool bEnvironmentCreated;
		FSteamAudioModule* SteamAudioModule;
	};
}
