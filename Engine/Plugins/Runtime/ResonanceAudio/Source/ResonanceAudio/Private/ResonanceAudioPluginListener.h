//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

#pragma once

#include <vraudio_api.h>

#include "AudioPluginUtilities.h"
#include "ResonanceAudioModule.h"
#include "ResonanceAudioReverb.h"
#include "ResonanceAudioSpatialization.h"

namespace ResonanceAudio
{
	// Dispatches listener information to the Resonance Audio plugins.
	class FResonanceAudioPluginListener : public IAudioPluginListener
	{
	public:
		FResonanceAudioPluginListener();
		~FResonanceAudioPluginListener();

		virtual void OnListenerInitialize(FAudioDevice* AudioDevice, UWorld* ListenerWorld) override;
		virtual void OnListenerUpdated(FAudioDevice* AudioDevice, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds) override;
		virtual void OnListenerShutdown(FAudioDevice* AudioDevice) override;
		virtual void OnTick(UWorld* InWorld, const int32 ViewportIndex, const FTransform& ListenerTransform, const float InDeltaSeconds) override;

	private:
		// Resonance Audio API instance.
		vraudio::VrAudioApi* ResonanceAudioApi;

		class FResonanceAudioModule* ResonanceAudioModule;
		class FResonanceAudioReverb* ReverbPtr;
		class FResonanceAudioSpatialization* SpatializationPtr;
		class FResonanceAudioAmbisonicsMixer* AmbisonicsPtr;
	};

} // namespace ResonanceAudio
