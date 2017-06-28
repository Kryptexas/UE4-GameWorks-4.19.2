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
#include "CommandLine.h"
#include "MessageDialog.h"
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
#include "Engine/StreamableManager.h"
#include "AudioDevice.h"

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

		if (FParse::Param(FCommandLine::Get(), TEXT("audiomixer")))
		{
			// We're overriding the IAudioSpatializationPlugin StartupModule, so we must be sure to register
			// the modular features. Without this line, we will not see SPATIALIZATION HRTF in the editor UI.
			IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
		}
		else
		{
			UE_LOG(LogSteamAudio, Warning, TEXT("Steam Audio requires that you run UE4 with the -audiomixer flag. See documentation for instructions."));
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Steam Audio requires that you run UE4 with the -audiomixer flag. See documentation for instructions."));
		}

		if (!FSteamAudioModule::PhononDllHandle)
		{
#if PLATFORM_32BITS
			FString PathToDll = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/Phonon/Win32/");
#else
			FString PathToDll = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/Phonon/Win64/");
#endif

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

		TArray<AActor*> PhononSceneActors;
		UGameplayStatics::GetAllActorsOfClass(World, APhononScene::StaticClass(), PhononSceneActors);

		if (PhononSceneActors.Num() == 0)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: PhononScene not found. Be sure to add a PhononScene actor to your level and export the scene."));
			return;
		}
		else if (PhononSceneActors.Num() > 1)
		{
			UE_LOG(LogSteamAudio, Warning, TEXT("More than one PhononScene actor found in level. Arbitrarily choosing one. Ensure only one exists to avoid unexpected behavior."));
		}

		APhononScene* PhononSceneActor = Cast<APhononScene>(PhononSceneActors[0]);
		check(PhononSceneActor);

		if (PhononSceneActor->SceneData.Num() == 0)
		{
			UE_LOG(LogSteamAudio, Error, TEXT("Unable to create Phonon environment: PhononScene actor does not have scene data. Be sure to export the scene."));
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
		RenderingSettings.samplingRate = OwningAudioDevice->GetSampleRate();

		EnvironmentalOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
		EnvironmentalOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
		EnvironmentalOutputAudioFormat.channelOrder = IPL_CHANNELORDER_DEINTERLEAVED;
		EnvironmentalOutputAudioFormat.numSpeakers = (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1);
		EnvironmentalOutputAudioFormat.speakerDirections = nullptr;
		EnvironmentalOutputAudioFormat.ambisonicsOrder = IndirectImpulseResponseOrder;
		EnvironmentalOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
		EnvironmentalOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

		IPLerror Error = IPLerror::IPL_STATUS_SUCCESS;

		iplLoadFinalizedScene(GlobalContext, SimulationSettings, PhononSceneActor->SceneData.GetData(), PhononSceneActor->SceneData.Num(), 
			ComputeDevice, nullptr, &PhononScene);
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

		Error = iplCreateEnvironmentalRenderer(GlobalContext, PhononEnvironment, RenderingSettings, EnvironmentalOutputAudioFormat,
			nullptr, nullptr, &EnvironmentalRenderer);
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

