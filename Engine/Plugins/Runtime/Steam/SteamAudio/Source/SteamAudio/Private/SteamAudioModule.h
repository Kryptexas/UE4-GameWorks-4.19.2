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

		virtual TAudioSpatializationPtr CreateSpatializationInterface(class FAudioDevice* AudioDevice) override;
		virtual TAudioOcclusionPtr CreateOcclusionInterface(class FAudioDevice* AudioDevice) override;
		virtual TAudioReverbPtr CreateReverbInterface(class FAudioDevice* AudioDevice) override;
		virtual TAudioListenerObserverPtr CreateListenerObserverInterface(class FAudioDevice* AudioDevice) override;

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

		TAudioSpatializationPtr SpatializationInstance;
		TAudioOcclusionPtr OcclusionInstance;
		TAudioReverbPtr ReverbInstance;
		TAudioListenerObserverPtr ListenerObserverInstance;

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