//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "SteamAudioEnvironment.h"
#include "PhononScene.h"
#include "PhononProbeVolume.h"
#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Classes/Kismet/GameplayStatics.h"
#include "IPluginManager.h"
#include "EngineUtils.h"
#include "Regex.h"
#include "ScopeLock.h"
#include "HAL/PlatformProcess.h"
#include "Engine/StreamableManager.h"
#include "AudioDevice.h"
#include "PhononScene.h"
#include "SteamAudioSettings.h"
#include "PhononProbeVolume.h"
#include "PlatformFileManager.h"
#include "GenericPlatformFile.h"
#include "Paths.h"

namespace SteamAudio
{

	FEnvironment::FEnvironment()
		: EnvironmentCriticalSection()
		, ComputeDevice(nullptr)
		, PhononScene(nullptr)
		, PhononEnvironment(nullptr)
		, EnvironmentalRenderer(nullptr)
		, ProbeManager(nullptr)
		, SimulationSettings()
		, RenderingSettings()
		, EnvironmentalOutputAudioFormat()
	{
	}

	FEnvironment::~FEnvironment()
	{
	}

	IPLhandle FEnvironment::Initialize(UWorld* World, FAudioDevice* InAudioDevice)
	{
		if (World == nullptr)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: null World."));
			return nullptr;
		}

		if (InAudioDevice == nullptr)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: null Audio Device."));
			return nullptr;
		}

		int32 IndirectImpulseResponseOrder = GetDefault<USteamAudioSettings>()->IndirectImpulseResponseOrder;
		float IndirectImpulseResponseDuration = GetDefault<USteamAudioSettings>()->IndirectImpulseResponseDuration;

		SimulationSettings.maxConvolutionSources = GetDefault<USteamAudioSettings>()->MaxSources;
		SimulationSettings.numBounces = GetDefault<USteamAudioSettings>()->RealtimeBounces;
		SimulationSettings.numDiffuseSamples = GetDefault<USteamAudioSettings>()->RealtimeSecondaryRays;
		SimulationSettings.numRays = GetDefault<USteamAudioSettings>()->RealtimeRays;
		SimulationSettings.ambisonicsOrder = IndirectImpulseResponseOrder;
		SimulationSettings.irDuration = IndirectImpulseResponseDuration;
		SimulationSettings.sceneType = IPL_SCENETYPE_PHONON;

		RenderingSettings.convolutionType = IPL_CONVOLUTIONTYPE_PHONON;
		RenderingSettings.frameSize = InAudioDevice->GetBufferLength();
		RenderingSettings.samplingRate = InAudioDevice->GetSampleRate();

		EnvironmentalOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		EnvironmentalOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		EnvironmentalOutputAudioFormat.channelOrder = IPL_CHANNELORDER_DEINTERLEAVED;
		EnvironmentalOutputAudioFormat.numSpeakers = (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1);
		EnvironmentalOutputAudioFormat.speakerDirections = nullptr;
		EnvironmentalOutputAudioFormat.ambisonicsOrder = IndirectImpulseResponseOrder;
		EnvironmentalOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		EnvironmentalOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		if (!LoadSceneFromDisk(World, ComputeDevice, SimulationSettings, &PhononScene, PhononSceneInfo))
		{
			UE_LOG(LogSteamAudio, Warning, TEXT("Unable to create Phonon environment: failed to load scene from disk. Be sure to export the scene."));
			return nullptr;
		}

		IPLerror Error = iplCreateProbeManager(&ProbeManager);
		LogSteamAudioStatus(Error);

		TArray<AActor*> PhononProbeVolumes;
		UGameplayStatics::GetAllActorsOfClass(World, APhononProbeVolume::StaticClass(), PhononProbeVolumes);

		for (AActor* PhononProbeVolumeActor : PhononProbeVolumes)
		{
			APhononProbeVolume* PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);

			if (PhononProbeVolume->GetProbeBatchDataSize() == 0)
			{
				UE_LOG(LogSteamAudio, Warning, TEXT("No batch data found on probe volume. You may need to bake indirect sound."));
				continue;
			}

			IPLhandle ProbeBatch = nullptr;
			PhononProbeVolume->LoadProbeBatchFromDisk(&ProbeBatch);

			iplAddProbeBatch(ProbeManager, ProbeBatch);

			ProbeBatches.Add(ProbeBatch);
		}

		Error = iplCreateEnvironment(GlobalContext, ComputeDevice, SimulationSettings, PhononScene, ProbeManager, &PhononEnvironment);
		LogSteamAudioStatus(Error);

		Error = iplCreateEnvironmentalRenderer(GlobalContext, PhononEnvironment, RenderingSettings, EnvironmentalOutputAudioFormat,
			nullptr, nullptr, &EnvironmentalRenderer);
		LogSteamAudioStatus(Error);

		return EnvironmentalRenderer;
	}

	void FEnvironment::Shutdown()
	{
		FScopeLock EnvironmentLock(&EnvironmentCriticalSection);
		if (ProbeManager)
		{
			for (IPLhandle ProbeBatch : ProbeBatches)
			{
				iplRemoveProbeBatch(ProbeManager, ProbeBatch);
				iplDestroyProbeBatch(&ProbeBatch);
			}

			ProbeBatches.Empty();

			iplDestroyProbeManager(&ProbeManager);
		}

		if (EnvironmentalRenderer)
		{
			iplDestroyEnvironmentalRenderer(&EnvironmentalRenderer);
		}

		if (PhononEnvironment)
		{
			iplDestroyEnvironment(&PhononEnvironment);
		}

		if (PhononScene)
		{
			iplDestroyScene(&PhononScene);
		}

		if (ComputeDevice)
		{
			iplDestroyComputeDevice(&ComputeDevice);
		}
	}

}