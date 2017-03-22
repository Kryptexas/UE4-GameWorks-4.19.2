// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononModule.h"

#include "PhononCommon.h"
#include "PhononSpatialization.h"
#include "PhononOcclusion.h"
#include "PhononReverb.h"
#include "PhononScene.h"
#include "PhononSettings.h"
#include "PhononProbeVolume.h"
#include "PhononListenerComponent.h"

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Classes/Kismet/GameplayStatics.h"
#include "IPluginManager.h"
#include "EngineUtils.h"
#include "Regex.h"

IMPLEMENT_MODULE(FPhononModule, FPhonon)

void* FPhononModule::PhononDllHandle = nullptr;

static bool bModuleStartedUp = false;

FPhononModule::FPhononModule()
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
	, BinauralRenderer(nullptr)
{

}

FPhononModule::~FPhononModule()
{
	check(!ComputeDevice);
	check(!PhononScene);
	check(!PhononEnvironment);
	check(!EnvironmentalRenderer);
	check(!ProbeManager);
	check(!BinauralRenderer);
}

void FPhononModule::StartupModule()
{
	check(bModuleStartedUp == false);

	bModuleStartedUp = true;

	UE_LOG(LogPhonon, Log, TEXT("FPhononModule Startup"));
	OwningAudioDevice = nullptr;

	SpatializationInstance = nullptr;
	OcclusionInstance = nullptr;
	ReverbInstance = nullptr;
	ComputeDevice = nullptr;
	PhononScene = nullptr;
	PhononEnvironment = nullptr;
	EnvironmentalRenderer = nullptr;
	ProbeManager = nullptr;
	BinauralRenderer = nullptr;
	SampleRate = 0;

	// We're overriding the IAudioSpatializationPlugin StartupModule, so we must be sure to register
	// the modular features. Without this line, we will not see SPATIALIZATION HRTF in the editor UI.
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);

	if (!FPhononModule::PhononDllHandle)
	{
		// TODO: do platform code to get the DLL location for the platform
		FString PathToDll = FPaths::EngineDir() / TEXT("Binaries/ThirdParty/Phonon/Win64/");


		FString DLLToLoad = PathToDll + TEXT("phonon.dll");
		FPhononModule::PhononDllHandle = Phonon::LoadDll(DLLToLoad);
	}

	iplCreateBinauralRenderer(Phonon::GlobalContext, RenderingSettings, nullptr, &BinauralRenderer);
}

void FPhononModule::ShutdownModule()
{
	UE_LOG(LogPhonon, Log, TEXT("FPhononModule Shutdown"));

	check(bModuleStartedUp == true);

	bModuleStartedUp = false;

	if (BinauralRenderer)
	{
		iplDestroyBinauralRenderer(&BinauralRenderer);
	}

	if (FPhononModule::PhononDllHandle)
	{
		FPlatformProcess::FreeDllHandle(FPhononModule::PhononDllHandle);
		FPhononModule::PhononDllHandle = nullptr;
	}
}

void FPhononModule::CreateEnvironment(UWorld* World, FAudioDevice* InAudioDevice)
{
	UE_LOG(LogPhonon, Log, TEXT("Creating Phonon environment..."));

	check(OwningAudioDevice == nullptr);
	OwningAudioDevice = InAudioDevice;

	int32 IndirectImpulseResponseOrder = GetDefault<UPhononSettings>()->IndirectImpulseResponseOrder;
	int32 IndirectImpulseResponseDuration = GetDefault<UPhononSettings>()->IndirectImpulseResponseDuration;

	SimulationSettings.maxConvolutionSources = 64;
	SimulationSettings.numBounces = GetDefault<UPhononSettings>()->RealtimeBounces;
	SimulationSettings.numDiffuseSamples = GetDefault<UPhononSettings>()->RealtimeSecondaryRays;
	SimulationSettings.numRays = GetDefault<UPhononSettings>()->RealtimeRays;
	SimulationSettings.ambisonicsOrder = IndirectImpulseResponseOrder;
	SimulationSettings.irDuration = IndirectImpulseResponseDuration;
	SimulationSettings.sceneType = IPL_SCENETYPE_PHONON;

	RenderingSettings.convolutionType = IPL_CONVOLUTIONTYPE_PHONON;
	RenderingSettings.frameSize = 1024; // FIXME
	RenderingSettings.samplingRate = AUDIO_SAMPLE_RATE; // FIXME

	EnvironmentalOutputAudioFormat.channelLayout = IPL_CHANNELLAYOUT_STEREO;
	EnvironmentalOutputAudioFormat.channelLayoutType = IPL_CHANNELLAYOUTTYPE_AMBISONICS;
	EnvironmentalOutputAudioFormat.channelOrder = IPL_CHANNELORDER_DEINTERLEAVED;
	EnvironmentalOutputAudioFormat.numSpeakers = (IndirectImpulseResponseOrder + 1) * (IndirectImpulseResponseOrder + 1);
	EnvironmentalOutputAudioFormat.speakerDirections = nullptr;
	EnvironmentalOutputAudioFormat.ambisonicsOrder = IndirectImpulseResponseOrder;
	EnvironmentalOutputAudioFormat.ambisonicsNormalization = IPL_AMBISONICSNORMALIZATION_N3D;
	EnvironmentalOutputAudioFormat.ambisonicsOrdering = IPL_AMBISONICSORDERING_ACN;

	if (World)
	{
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

		if (FPaths::FileExists(SceneFile))
		{
			IPLerror Error = IPLerror::IPL_STATUS_SUCCESS;

			Error = iplLoadFinalizedScene(Phonon::GlobalContext, SimulationSettings, TCHAR_TO_ANSI(*SceneFile), ComputeDevice, nullptr, &PhononScene);
			Phonon::LogPhononStatus(Error);

			Error = iplCreateProbeManager(&ProbeManager);
			Phonon::LogPhononStatus(Error);

			TArray<AActor*> PhononProbeVolumes;
			UGameplayStatics::GetAllActorsOfClass(World, APhononProbeVolume::StaticClass(), PhononProbeVolumes);

			for (AActor* PhononProbeVolumeActor : PhononProbeVolumes)
			{
				APhononProbeVolume* PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);
				IPLhandle ProbeBatch = nullptr;
				Error = iplLoadProbeBatch(PhononProbeVolume->GetProbeBatchData(), PhononProbeVolume->GetProbeBatchDataSize(), &ProbeBatch);
				Phonon::LogPhononStatus(Error);

				iplAddProbeBatch(ProbeManager, ProbeBatch);

				ProbeBatches.Add(ProbeBatch);
			}

			Error = iplCreateEnvironment(Phonon::GlobalContext, ComputeDevice, SimulationSettings, PhononScene, ProbeManager, &PhononEnvironment);
			Phonon::LogPhononStatus(Error);

			Error = iplCreateEnvironmentalRenderer(Phonon::GlobalContext, PhononEnvironment, RenderingSettings, EnvironmentalOutputAudioFormat, &EnvironmentalRenderer);
			Phonon::LogPhononStatus(Error);

			Phonon::FPhononOcclusion* PhononOcclusion = static_cast<Phonon::FPhononOcclusion*>(OcclusionInstance.Get());
			PhononOcclusion->SetEnvironmentalRenderer(EnvironmentalRenderer);

			Phonon::FPhononReverb* PhononReverb = static_cast<Phonon::FPhononReverb*>(ReverbInstance.Get());
			PhononReverb->SetEnvironmentalRenderer(EnvironmentalRenderer);
		}
		else
		{
			UE_LOG(LogPhonon, Log, TEXT("Unable to load scene: %s not found."), *SceneFile);
		}
	}
	else
	{
		UE_LOG(LogPhonon, Log, TEXT("Unable to load scene: null World."));
	}
}

void FPhononModule::DestroyEnvironment(FAudioDevice* InAudioDevice)
{
	check(OwningAudioDevice == InAudioDevice);

	OwningAudioDevice = nullptr;

	if (ProbeManager)
	{
		for (IPLhandle ProbeBatch : ProbeBatches)
		{
			iplRemoveProbeBatch(ProbeManager, ProbeBatch);
			iplDestroyProbeBatch(&ProbeBatch);
		}

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

void FPhononModule::SetSampleRate(const int32 InSampleRate)
{
	SampleRate = InSampleRate;
}

TSharedPtr<IAudioSpatialization> FPhononModule::CreateSpatializationInterface(class FAudioDevice* AudioDevice)
{
	return SpatializationInstance = TSharedPtr<IAudioSpatialization>(new Phonon::FPhononSpatialization());
}

TSharedPtr<IAudioOcclusion> FPhononModule::CreateOcclusionInterface(class FAudioDevice* AudioDevice)
{
	return OcclusionInstance = TSharedPtr<IAudioOcclusion>(new Phonon::FPhononOcclusion());
}

TSharedPtr<IAudioReverb> FPhononModule::CreateReverbInterface(class FAudioDevice* AudioDevice)
{
	return ReverbInstance  = TSharedPtr<IAudioReverb>(new Phonon::FPhononReverb());
}

TSharedPtr<IAudioListenerObserver> FPhononModule::CreateListenerObserverInterface(class FAudioDevice* AudioDevice)
{
	return ListenerObserverInstance = TSharedPtr<IAudioListenerObserver>(new Phonon::FPhononListenerObserver());
}

Phonon::FPhononSpatialization* FPhononModule::GetSpatializationInstance() const
{
	return static_cast<Phonon::FPhononSpatialization*>(SpatializationInstance.Get());
}

Phonon::FPhononOcclusion* FPhononModule::GetOcclusionInstance() const
{
	return static_cast<Phonon::FPhononOcclusion*>(OcclusionInstance.Get());
}

Phonon::FPhononReverb* FPhononModule::GetReverbInstance() const
{
	return static_cast<Phonon::FPhononReverb*>(ReverbInstance.Get());
}

Phonon::FPhononListenerObserver* FPhononModule::GetListenerObserverInstance() const
{
	return static_cast<Phonon::FPhononListenerObserver*>(ListenerObserverInstance.Get());
}

