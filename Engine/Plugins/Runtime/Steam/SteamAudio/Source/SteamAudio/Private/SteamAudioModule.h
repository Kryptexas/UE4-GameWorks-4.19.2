//
// Copyright (C) Valve Corporation. All rights reserved.
//

#pragma once

#include "ISteamAudioModule.h"
#include "phonon.h"

namespace SteamAudio
{
	class FSteamAudioModule : public ISteamAudioModule
	{
	public:
		FSteamAudioModule();
		~FSteamAudioModule();

		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

		virtual bool ImplementsSpatialization() const override;
		virtual bool ImplementsOcclusion() const override;
		virtual bool ImplementsReverb() const override;

		virtual TSharedPtr<IAudioSpatialization> CreateSpatializationInterface(class FAudioDevice* AudioDevice) override;
		virtual TSharedPtr<IAudioOcclusion> CreateOcclusionInterface(class FAudioDevice* AudioDevice) override;
		virtual TSharedPtr<IAudioReverb> CreateReverbInterface(class FAudioDevice* AudioDevice) override;
		virtual TSharedPtr<IAudioListenerObserver> CreateListenerObserverInterface(class FAudioDevice* AudioDevice) override;

		void CreateEnvironment(UWorld* World, FAudioDevice* InAudioDevice);
		void DestroyEnvironment(FAudioDevice* InAudioDevice);

		void SetSampleRate(const int32 SampleRate);
		FCriticalSection& GetEnvironmentCriticalSection();
		class FPhononSpatialization* GetSpatializationInstance() const;
		class FPhononOcclusion* GetOcclusionInstance() const;
		class FPhononReverb* GetReverbInstance() const;
		class FPhononListenerObserver* GetListenerObserverInstance() const;

	private:
		static void* PhononDllHandle;

		TSharedPtr<IAudioSpatialization> SpatializationInstance;
		TSharedPtr<IAudioOcclusion> OcclusionInstance;
		TSharedPtr<IAudioReverb> ReverbInstance;
		TSharedPtr<IAudioListenerObserver> ListenerObserverInstance;

		int32 SampleRate;
		FAudioDevice* OwningAudioDevice;

		FCriticalSection EnvironmentCriticalSection;
		IPLhandle ComputeDevice;
		IPLhandle PhononScene;
		IPLhandle PhononEnvironment;
		IPLhandle EnvironmentalRenderer;
		IPLhandle ProbeManager;
		TArray<IPLhandle> ProbeBatches;
		IPLSimulationSettings SimulationSettings;
		IPLRenderingSettings RenderingSettings;
		IPLAudioFormat EnvironmentalOutputAudioFormat;
	};
}