//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "SteamAudioModule.h"

#include "PhononCommon.h"
#include "PhononSpatialization.h"
#include "PhononOcclusion.h"
#include "PhononReverb.h"
#include "PhononScene.h"
#include "SteamAudioSettings.h"
#include "PhononProbeVolume.h"
#include "PhononListenerObserver.h"

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Classes/Kismet/GameplayStatics.h"
#include "IPluginManager.h"
#include "EngineUtils.h"
#include "Regex.h"
#include "ScopeLock.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

IMPLEMENT_MODULE(SteamAudio::FSteamAudioModule, SteamAudio)

namespace SteamAudio
{
	void* FSteamAudioModule::PhononDllHandle = nullptr;

	static bool bModuleStartedUp = false;

	FSteamAudioModule::FSteamAudioModule()
		: SpatializationInstance(nullptr)
		, OcclusionInstance(nullptr)
		, ReverbInstance(nullptr)
		, ListenerObserverInstance(nullptr)
		, SampleRate(0.0f)
		, OwningAudioDevice(nullptr)
		, ComputeDevice(nullptr)
		, PhononScene(nullptr)
		, PhononEnvironment(nullptr)
		, EnvironmentalRenderer(nullptr)
		, ProbeManager(nullptr)
	{
	}

	FSteamAudioModule::~FSteamAudioModule()
	{
		check(!ComputeDevice);
		check(!PhononScene);
		check(!PhononEnvironment);
		check(!EnvironmentalRenderer);
		check(!ProbeManager);
	}

	void FSteamAudioModule::StartupModule()
	{
		check(bModuleStartedUp == false);

		bModuleStartedUp = true;

		UE_LOG(LogSteamAudio, Log, TEXT("FSteamAudioModule Startup"));
		OwningAudioDevice = nullptr;

		SpatializationInstance = nullptr;
		OcclusionInstance = nullptr;
		ReverbInstance = nullptr;
		ComputeDevice = nullptr;
		PhononScene = nullptr;
		PhononEnvironment = nullptr;
		EnvironmentalRenderer = nullptr;
		ProbeManager = nullptr;
		SampleRate = 0;

		// We're overriding the IAudioSpatializationPlugin StartupModule, so we must be sure to register
		// the modular features. Without this line, we will not see SPATIALIZATION HRTF in the editor UI.
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

		if (!FSteamAudioModule::PhononDllHandle)
		{
			// TODO: do platform code to get the DLL location for the platform
			FString PathToDll = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/Phonon/Win64/");

			FString DLLToLoad = PathToDll + TEXT("phonon.dll");
			FSteamAudioModule::PhononDllHandle = LoadDll(DLLToLoad);
		}
	}

	void FSteamAudioModule::ShutdownModule()
	{
		UE_LOG(LogSteamAudio, Log, TEXT("FSteamAudioModule Shutdown"));

		check(bModuleStartedUp == true);

		bModuleStartedUp = false;

		if (FSteamAudioModule::PhononDllHandle)
		{
			FPlatformProcess::FreeDllHandle(FSteamAudioModule::PhononDllHandle);
			FSteamAudioModule::PhononDllHandle = nullptr;
		}
	}

	void FSteamAudioModule::CreateEnvironment(UWorld* World, FAudioDevice* InAudioDevice)
	{
		if (World == nullptr)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: null World."));
			return;
		}

		if (InAudioDevice == nullptr)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: null Audio Device."));
			return;
		}

		check(OwningAudioDevice == nullptr);
		OwningAudioDevice = InAudioDevice;

		FString MapName = World->GetMapName();

		// Strip out the PIE prefix for the map name before looking for the phononscene.
		const FRegexPattern PieMapPattern(TEXT("UEDPIE_\\d_"));
		FRegexMatcher Matcher(PieMapPattern, MapName);

		// Look for the PIE pattern
		if (Matcher.FindNext())
		{
			int32 Beginning = Matcher.GetMatchBeginning();
			int32 Ending = Matcher.GetMatchEnding();
			MapName = MapName.Mid(Ending, MapName.Len());
		}

		FString SceneFile = FPaths::GameDir() + "Content/" + MapName + ".phononscene";

		if (!FPaths::FileExists(SceneFile))
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: Phonon scene not found. Be sure to export the scene."));
			return;
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
		RenderingSettings.frameSize = 1024; // FIXME
		RenderingSettings.samplingRate = AUDIO_SAMPLE_RATE;

		EnvironmentalOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		EnvironmentalOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		EnvironmentalOutputAudioFormat.channelOrder = IPL_CHANNELORDER_DEINTERLEAVED;
		EnvironmentalOutputAudioFormat.numSpeakers = (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1);
		EnvironmentalOutputAudioFormat.speakerDirections = nullptr;
		EnvironmentalOutputAudioFormat.ambisonicsOrder = IndirectImpulseResponseOrder;
		EnvironmentalOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		EnvironmentalOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		IPLerror Error = IPLerror::IPL_STATUS_SUCCESS;

		Error = iplLoadFinalizedScene(GlobalContext, SimulationSettings, TCHAR_TO_ANSI(*SceneFile), ComputeDevice, nullptr, &PhononScene);
		LogSteamAudioStatus(Error);

		Error = iplCreateProbeManager(&ProbeManager);
		LogSteamAudioStatus(Error);

		TArray<AActor*> PhononProbeVolumes;
		UGameplayStatics::GetAllActorsOfClass(World, APhononProbeVolume::StaticClass(), PhononProbeVolumes);

		for (AActor* PhononProbeVolumeActor : PhononProbeVolumes)
		{
			APhononProbeVolume* PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);
			IPLhandle ProbeBatch = nullptr;
			Error = iplLoadProbeBatch(PhononProbeVolume->GetProbeBatchData(), PhononProbeVolume->GetProbeBatchDataSize(), &ProbeBatch);
			LogSteamAudioStatus(Error);

			iplAddProbeBatch(ProbeManager, ProbeBatch);

			ProbeBatches.Add(ProbeBatch);
		}

		Error = iplCreateEnvironment(GlobalContext, ComputeDevice, SimulationSettings, PhononScene, ProbeManager, &PhononEnvironment);
		LogSteamAudioStatus(Error);

		Error = iplCreateEnvironmentalRenderer(GlobalContext, PhononEnvironment, RenderingSettings, EnvironmentalOutputAudioFormat, &EnvironmentalRenderer);
		LogSteamAudioStatus(Error);

		FPhononOcclusion* PhononOcclusion = static_cast<FPhononOcclusion*>(OcclusionInstance.Get());
		PhononOcclusion->SetEnvironmentalRenderer(EnvironmentalRenderer);

		FPhononReverb* PhononReverb = static_cast<FPhononReverb*>(ReverbInstance.Get());
		PhononReverb->SetEnvironmentalRenderer(EnvironmentalRenderer);
		PhononReverb->CreateReverbEffect();
	}

	void FSteamAudioModule::DestroyEnvironment(FAudioDevice* InAudioDevice)
	{
		check(OwningAudioDevice == InAudioDevice);

		OwningAudioDevice = nullptr;

		FScopeLock Lock(&EnvironmentCriticalSection);

		FPhononOcclusion* PhononOcclusion = static_cast<FPhononOcclusion*>(OcclusionInstance.Get());
		PhononOcclusion->SetEnvironmentalRenderer(nullptr);

		FPhononReverb* PhononReverb = static_cast<FPhononReverb*>(ReverbInstance.Get());
		PhononReverb->SetEnvironmentalRenderer(nullptr);

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

	bool FSteamAudioModule::ImplementsSpatialization() const
	{
		return true;
	}

	bool FSteamAudioModule::ImplementsOcclusion() const
	{
		return true;
	}

	bool FSteamAudioModule::ImplementsReverb() const
	{
		return true;
	}

	TSharedPtr<IAudioSpatialization> FSteamAudioModule::CreateSpatializationInterface(class FAudioDevice* AudioDevice)
	{
		return SpatializationInstance = TSharedPtr<IAudioSpatialization>(new FPhononSpatialization());
	}

	TSharedPtr<IAudioOcclusion> FSteamAudioModule::CreateOcclusionInterface(class FAudioDevice* AudioDevice)
	{
		return OcclusionInstance = TSharedPtr<IAudioOcclusion>(new FPhononOcclusion());
	}

	TSharedPtr<IAudioReverb> FSteamAudioModule::CreateReverbInterface(class FAudioDevice* AudioDevice)
	{
		return ReverbInstance = TSharedPtr<IAudioReverb>(new FPhononReverb());
	}

	TSharedPtr<IAudioListenerObserver> FSteamAudioModule::CreateListenerObserverInterface(class FAudioDevice* AudioDevice)
	{
		return ListenerObserverInstance = TSharedPtr<IAudioListenerObserver>(new FPhononListenerObserver());
	}

	void FSteamAudioModule::SetSampleRate(const int32 InSampleRate)
	{
		SampleRate = InSampleRate;
	}

	FPhononSpatialization* FSteamAudioModule::GetSpatializationInstance() const
	{
		return static_cast<FPhononSpatialization*>(SpatializationInstance.Get());
	}

	FPhononOcclusion* FSteamAudioModule::GetOcclusionInstance() const
	{
		return static_cast<FPhononOcclusion*>(OcclusionInstance.Get());
	}

	FPhononReverb* FSteamAudioModule::GetReverbInstance() const
	{
		return static_cast<FPhononReverb*>(ReverbInstance.Get());
	}

	FPhononListenerObserver* FSteamAudioModule::GetListenerObserverInstance() const
	{
		return static_cast<FPhononListenerObserver*>(ListenerObserverInstance.Get());
	}

	FCriticalSection& FSteamAudioModule::GetEnvironmentCriticalSection()
	{
		return EnvironmentCriticalSection;
	}
}

