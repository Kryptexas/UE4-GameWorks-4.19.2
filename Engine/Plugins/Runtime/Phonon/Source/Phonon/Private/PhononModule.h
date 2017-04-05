//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "IPhononModule.h"
#include <atomic>
#include "phonon.h"

namespace Phonon
{
	class FPhononSpatialization;
	class FPhononOcclusion;
	class FPhononReverb;
	class FPhononListenerObserver;
}

class FPhononModule : public IPhononModule
{
public:
	FPhononModule();
	~FPhononModule();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual TSharedPtr<IAudioSpatialization> CreateSpatializationInterface(class FAudioDevice* AudioDevice) override;
	virtual TSharedPtr<IAudioOcclusion> CreateOcclusionInterface(class FAudioDevice* AudioDevice) override;
	virtual TSharedPtr<IAudioReverb> CreateReverbInterface(class FAudioDevice* AudioDevice) override;
	virtual TSharedPtr<IAudioListenerObserver> CreateListenerObserverInterface(class FAudioDevice* AudioDevice) override;

	void CreateEnvironment(UWorld* World, FAudioDevice* InAudioDevice);
	void DestroyEnvironment(FAudioDevice* InAudioDevice);

	void SetSampleRate(const int32 SampleRate);

	virtual bool SupportsMultipleAudioDevices() const { return false; }
	virtual bool ImplementsSpatialization() const { return true; }
	virtual bool ImplementsOcclusion() const { return true; }
	virtual bool ImplementsReverb() const { return true; }

	Phonon::FPhononSpatialization* GetSpatializationInstance() const;
	Phonon::FPhononOcclusion* GetOcclusionInstance() const;
	Phonon::FPhononReverb* GetReverbInstance() const;
	Phonon::FPhononListenerObserver* GetListenerObserverInstance() const;

private:

	TSharedPtr<IAudioSpatialization> SpatializationInstance;
	TSharedPtr<IAudioOcclusion> OcclusionInstance;
	TSharedPtr<IAudioReverb> ReverbInstance;
	TSharedPtr<IAudioListenerObserver> ListenerObserverInstance;

	int32 SampleRate;

	FAudioDevice* OwningAudioDevice;

	IPLhandle ComputeDevice;
	IPLhandle PhononScene;
	IPLhandle PhononEnvironment;
	IPLhandle EnvironmentalRenderer;
	IPLhandle ProbeManager;
	IPLhandle BinauralRenderer;
	TArray<IPLhandle> ProbeBatches;
	IPLSimulationSettings SimulationSettings;
	IPLRenderingSettings RenderingSettings;

	IPLAudioFormat EnvironmentalOutputAudioFormat;

	// TODO: make this an instance var
	static void* PhononDllHandle;
};