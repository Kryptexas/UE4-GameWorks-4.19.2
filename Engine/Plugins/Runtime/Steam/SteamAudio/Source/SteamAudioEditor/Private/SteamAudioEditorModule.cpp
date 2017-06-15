//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "SteamAudioEditorModule.h"

#include "ISettingsModule.h"
#include "Engine/World.h"
#include "SlateStyleRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "LevelEditorViewport.h"
#include "LevelEditor.h"
#include "PropertyEditorModule.h"
#include "SlateStyle.h"
#include "IPluginManager.h"
#include "ClassIconFinder.h"
#include "Editor.h"

#include "TickableNotification.h"
#include "PhononScene.h"
#include "SteamAudioSettings.h"
#include "PhononProbeVolume.h"
#include "PhononProbeVolumeDetails.h"
#include "PhononProbeComponent.h"
#include "PhononProbeComponentVisualizer.h"
#include "PhononSourceComponent.h"
#include "PhononSourceComponentDetails.h"
#include "PhononSourceComponentVisualizer.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include <atomic>

DEFINE_LOG_CATEGORY(LogSteamAudioEditor);

IMPLEMENT_MODULE(SteamAudio::FSteamAudioEditorModule, SteamAudioEditor)

namespace SteamAudio
{
	//==============================================================================================================================================
	// Global data and functions for baking
	//==============================================================================================================================================

	static TSharedPtr<SteamAudio::FTickableNotification> GExportSceneTickable = MakeShareable(new FTickableNotification());
	static TSharedPtr<SteamAudio::FTickableNotification> GBakeReverbTickable = MakeShareable(new FTickableNotification());
	static std::atomic<bool> GIsBakingReverb = false;
	static int32 GCurrentProbeVolume = 0;
	static int32 GNumProbeVolumes = 0;

	static void BakeReverbProgressCallback(float Progress)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("BakeProgress"), FText::AsPercent(Progress));
		Arguments.Add(TEXT("CurrentProbeVolume"), FText::AsNumber(GCurrentProbeVolume));
		Arguments.Add(TEXT("NumProbeVolumes"), FText::AsNumber(GNumProbeVolumes));
		GBakeReverbTickable->SetDisplayText(FText::Format(NSLOCTEXT("SteamAudio", "BakeText",
			"Baking {CurrentProbeVolume}/{NumProbeVolumes} probe volumes ({BakeProgress} complete)"), Arguments));
	}

	//==============================================================================================================================================
	// FSteamAudioEditorModule
	//==============================================================================================================================================

	void FSteamAudioEditorModule::StartupModule()
	{
		// Register detail customizations
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.RegisterCustomClassLayout("PhononProbeVolume", FOnGetDetailCustomizationInstance::CreateStatic(&FPhononProbeVolumeDetails::MakeInstance));
		PropertyModule.RegisterCustomClassLayout("PhononSourceComponent",  FOnGetDetailCustomizationInstance::CreateStatic(&FPhononSourceComponentDetails::MakeInstance));

		// Extend the toolbar build menu with custom actions
		if (!IsRunningCommandlet())
		{
			FLevelEditorModule* LevelEditorModule = FModuleManager::LoadModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
			if (LevelEditorModule)
			{
				auto BuildMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FSteamAudioEditorModule::OnExtendLevelEditorBuildMenu);

				LevelEditorModule->GetAllLevelEditorToolbarBuildMenuExtenders().Add(BuildMenuExtender);
			}
		}

		// Register plugin settings
		ISettingsModule* SettingsModule = FModuleManager::Get().GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			SettingsModule->RegisterSettings("Project", "Plugins", "Steam Audio", NSLOCTEXT("SteamAudio", "Steam Audio", "Steam Audio"),
				NSLOCTEXT("SteamAudio", "Configure Steam Audio settings", "Configure Steam Audio settings"), GetMutableDefault<USteamAudioSettings>());
		}

		// Create and register custom slate style
		FString SteamAudioContent = IPluginManager::Get().FindPlugin("SteamAudio")->GetBaseDir() + "/Content";
		FVector2D Vec16 = FVector2D(16.0f, 16.0f);
		FVector2D Vec64 = FVector2D(64.0f, 64.0f);

		SteamAudioStyleSet = MakeShareable(new FSlateStyleSet("SteamAudio"));
		SteamAudioStyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
		SteamAudioStyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));
		SteamAudioStyleSet->Set("ClassIcon.PhononSourceComponent", new FSlateImageBrush(SteamAudioContent + "/S_PhononSource_16.png", Vec16));
		SteamAudioStyleSet->Set("ClassIcon.PhononGeometryComponent", new FSlateImageBrush(SteamAudioContent + "/S_PhononGeometry_16.png", Vec16));
		SteamAudioStyleSet->Set("ClassIcon.PhononMaterialComponent", new FSlateImageBrush(SteamAudioContent + "/S_PhononMaterial_16.png", Vec16));

		SteamAudioStyleSet->Set("ClassIcon.PhononSpatializationSourceSettings", new FSlateImageBrush(SteamAudioContent + "/S_PhononSpatializationSourceSettings_16.png", Vec16));
		SteamAudioStyleSet->Set("ClassThumbnail.PhononSpatializationSourceSettings", new FSlateImageBrush(SteamAudioContent + "/S_PhononSpatializationSourceSettings_64.png", Vec64));
		
		SteamAudioStyleSet->Set("ClassIcon.PhononOcclusionSourceSettings", new FSlateImageBrush(SteamAudioContent + "/S_PhononOcclusionSourceSettings_16.png", Vec16));
		SteamAudioStyleSet->Set("ClassThumbnail.PhononOcclusionSourceSettings", new FSlateImageBrush(SteamAudioContent + "/S_PhononOcclusionSourceSettings_64.png", Vec64));
		
		SteamAudioStyleSet->Set("ClassIcon.PhononReverbSourceSettings", new FSlateImageBrush(SteamAudioContent + "/S_PhononReverbSourceSettings_16.png", Vec16));
		SteamAudioStyleSet->Set("ClassThumbnail.PhononReverbSourceSettings", new FSlateImageBrush(SteamAudioContent + "/S_PhononReverbSourceSettings_64.png", Vec64));
		FSlateStyleRegistry::RegisterSlateStyle(*SteamAudioStyleSet.Get());

		// Register component visualizers
		RegisterComponentVisualizer(UPhononSourceComponent::StaticClass()->GetFName(), MakeShareable(new FPhononSourceComponentVisualizer()));
		RegisterComponentVisualizer(UPhononProbeComponent::StaticClass()->GetFName(), MakeShareable(new FPhononProbeComponentVisualizer()));
	}

	void FSteamAudioEditorModule::ShutdownModule()
	{
		// Unregister component visualizers
		if (GUnrealEd)
		{
			for (FName& ClassName : RegisteredComponentClassNames)
			{
				GUnrealEd->UnregisterComponentVisualizer(ClassName);
			}
		}

		FSlateStyleRegistry::UnRegisterSlateStyle(*SteamAudioStyleSet.Get());
	}

	void FSteamAudioEditorModule::ExportScene()
	{
		// Display editor notification
		GExportSceneTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Exporting scene...", "Exporting scene..."));
		GExportSceneTickable->CreateNotification();

		// Export the scene
		UWorld* World = GEditor->LevelViewportClients[0]->GetWorld();
		IPLhandle PhononScene = nullptr;
		TArray<IPLhandle> PhononStaticMeshes;
		SteamAudio::LoadScene(World, &PhononScene, &PhononStaticMeshes);

		// Save to disk
		FString BaseSceneFile = FPaths::GameDir() + "Content/" + World->GetMapName();
		FString SceneFile = BaseSceneFile + ".phononscene";
		FString ObjSceneFile = BaseSceneFile + ".obj";
		iplSaveFinalizedScene(PhononScene, TCHAR_TO_ANSI(*SceneFile));
		iplDumpSceneToObjFile(PhononScene, TCHAR_TO_ANSI(*ObjSceneFile));

		// Clean up Phonon structures
		for (IPLhandle PhononStaticMesh : PhononStaticMeshes)
		{
			iplDestroyStaticMesh(&PhononStaticMesh);
		}
		iplDestroyScene(&PhononScene);

		// Display editor notification
		GExportSceneTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Export scene complete.", "Export scene complete."));
		GExportSceneTickable->DestroyNotification();
	}

	void FSteamAudioEditorModule::BakeReverb()
	{
		GIsBakingReverb.store(true);

		// Display editor notification
		GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Baking reverb...", "Baking reverb..."));
		GBakeReverbTickable->CreateNotificationWithCancel(FSimpleDelegate::CreateRaw(this, &FSteamAudioEditorModule::CancelBakeReverb));

		UWorld* World = GEditor->LevelViewportClients[0]->GetWorld();

		// Get all probe volumes (cannot do this in the async task)
		TArray<AActor*> PhononProbeVolumes;
		UGameplayStatics::GetAllActorsOfClass(World, APhononProbeVolume::StaticClass(), PhononProbeVolumes);

		// Ensure we have at least one probe
		bool AtLeastOneProbe = false;

		for (auto PhononProbeVolumeActor : PhononProbeVolumes)
		{
			auto PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);
			if (PhononProbeVolume->NumProbes > 0)
			{
				AtLeastOneProbe = true;
				break;
			}
		}

		if (!AtLeastOneProbe)
		{
			UE_LOG(LogSteamAudioEditor, Error, TEXT("Ensure at least one Phonon Probe Volume with probes exists."));
			GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake failed.", "Bake failed. Create at least one Phonon Probe Volume that has probes."));
			GBakeReverbTickable->DestroyNotification(SNotificationItem::CS_Fail);
			GIsBakingReverb.store(false);
			return;
		}

		AsyncTask(ENamedThreads::AnyNormalThreadNormalTask, [=]()
		{
			IPLBakingSettings BakingSettings;
			BakingSettings.bakeParametric = IPL_FALSE;
			BakingSettings.bakeConvolution = IPL_TRUE;

			IPLSimulationSettings SimulationSettings;
			SimulationSettings.sceneType = IPL_SCENETYPE_PHONON;
			SimulationSettings.irDuration = GetDefault<USteamAudioSettings>()->IndirectImpulseResponseDuration;
			SimulationSettings.ambisonicsOrder = GetDefault<USteamAudioSettings>()->IndirectImpulseResponseOrder;
			SimulationSettings.maxConvolutionSources = 0;
			SimulationSettings.numBounces = GetDefault<USteamAudioSettings>()->BakedBounces;
			SimulationSettings.numRays = GetDefault<USteamAudioSettings>()->BakedRays;
			SimulationSettings.numDiffuseSamples = GetDefault<USteamAudioSettings>()->BakedSecondaryRays;

			IPLhandle ComputeDevice = nullptr;
			IPLLoadSceneProgressCallback LoadSceneProgressCallback = nullptr;

			IPLhandle PhononScene = nullptr;
			IPLhandle PhononEnvironment = nullptr;

			GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Loading scene...", "Loading scene..."));

			if (World)
			{
				FString SceneFile = FPaths::GameDir() + "Content/" + World->GetMapName() + ".phononscene";

				if (FPaths::FileExists(SceneFile))
				{
					iplLoadFinalizedScene(SteamAudio::GlobalContext, SimulationSettings, TCHAR_TO_ANSI(*SceneFile), ComputeDevice, LoadSceneProgressCallback, &PhononScene);
					iplCreateEnvironment(SteamAudio::GlobalContext, ComputeDevice, SimulationSettings, PhononScene, nullptr, &PhononEnvironment);
				}
				else
				{
					UE_LOG(LogSteamAudioEditor, Error, TEXT("Unable to load scene: %s not found."), *SceneFile);
					GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake failed.", "Bake failed. Export scene first."));
					GBakeReverbTickable->DestroyNotification(SNotificationItem::CS_Fail);
					GIsBakingReverb.store(false);
					return;
				}
			}
			else
			{
				UE_LOG(LogSteamAudioEditor, Error, TEXT("Unable to load scene: null World."));
				GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake failed.", "Bake failed. Export scene first."));
				GBakeReverbTickable->DestroyNotification(SNotificationItem::CS_Fail);
				GIsBakingReverb.store(false);
				return;
			}

			GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Baking reverb...", "Baking reverb..."));
			GNumProbeVolumes = PhononProbeVolumes.Num();
			GCurrentProbeVolume = 1;

			for (AActor* PhononProbeVolumeActor : PhononProbeVolumes)
			{
				APhononProbeVolume* PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);
				IPLhandle ProbeBox = nullptr;
				iplLoadProbeBox(PhononProbeVolume->GetProbeBoxData(), PhononProbeVolume->GetProbeBoxDataSize(), &ProbeBox);
				iplBakeReverb(PhononEnvironment, ProbeBox, BakingSettings, BakeReverbProgressCallback);

				FBakedDataInfo BakedDataInfo;
				BakedDataInfo.Name = "__reverb__";
				BakedDataInfo.Size = iplGetBakedDataSizeByName(ProbeBox, (IPLstring)"__reverb__");
				
				auto ExistingInfo = PhononProbeVolume->BakedDataInfo.FindByPredicate([=](const FBakedDataInfo& InfoItem)
				{
					return InfoItem.Name == BakedDataInfo.Name;
				});

				if (ExistingInfo)
				{
					ExistingInfo->Size = BakedDataInfo.Size;
				}
				else
				{
					PhononProbeVolume->BakedDataInfo.Add(BakedDataInfo);
					PhononProbeVolume->BakedDataInfo.Sort();
				}

				if (!GIsBakingReverb.load())
				{
					iplDestroyEnvironment(&PhononEnvironment);
					iplDestroyScene(&PhononScene);
					GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake reverb cancelled.", "Bake reverb cancelled."));
					GBakeReverbTickable->DestroyNotification(SNotificationItem::CS_Fail);
					return;
				}

				PhononProbeVolume->UpdateProbeBoxData(ProbeBox);
				iplDestroyProbeBox(&ProbeBox);
				++GCurrentProbeVolume;
			}

			iplDestroyEnvironment(&PhononEnvironment);
			iplDestroyScene(&PhononScene);

			// Display editor notification
			GBakeReverbTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake reverb complete.", "Bake reverb complete."));
			GBakeReverbTickable->DestroyNotification();
			GIsBakingReverb.store(false);
		});
	}

	void FSteamAudioEditorModule::RegisterComponentVisualizer(const FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer)
	{
		if (GUnrealEd)
		{
			GUnrealEd->RegisterComponentVisualizer(ComponentClassName, Visualizer);
		}

		RegisteredComponentClassNames.Add(ComponentClassName);

		if (Visualizer.IsValid())
		{
			Visualizer->OnRegister();
		}
	}

	TSharedRef<FExtender> FSteamAudioEditorModule::OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		Extender->AddMenuExtension("LevelEditorNavigation", EExtensionHook::After, nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FSteamAudioEditorModule::CreateBuildMenu));

		return Extender;
	}

	void FSteamAudioEditorModule::CreateBuildMenu(FMenuBuilder& Builder)
	{
		FUIAction ActionExportScene(FExecuteAction::CreateRaw(this, &FSteamAudioEditorModule::ExportScene),
			FCanExecuteAction::CreateRaw(this, &FSteamAudioEditorModule::IsReadyToExportScene));

		FUIAction ActionBakeReverb(FExecuteAction::CreateRaw(this, &FSteamAudioEditorModule::BakeReverb),
			FCanExecuteAction::CreateRaw(this, &FSteamAudioEditorModule::IsReadyToBakeReverb));

		Builder.BeginSection("LevelEditorIR", NSLOCTEXT("SteamAudio", "Phonon", "Phonon"));

		Builder.AddMenuEntry(NSLOCTEXT("SteamAudio", "Export Scene", "Export Scene"),
			NSLOCTEXT("SteamAudio", "Exports Phonon geometry.", "Export Phonon geometry."), FSlateIcon(), ActionExportScene, NAME_None,
			EUserInterfaceActionType::Button);

		Builder.AddMenuEntry(NSLOCTEXT("SteamAudio", "Bake Reverb", "Bake Reverb"),
			NSLOCTEXT("SteamAudio", "Bakes reverb at all probe locations.", "Bakes reverb at all probe locations."), FSlateIcon(), ActionBakeReverb,
			NAME_None, EUserInterfaceActionType::Button);

		Builder.EndSection();
	}

	void FSteamAudioEditorModule::CancelBakeReverb()
	{
		iplCancelBake();
		GIsBakingReverb.store(false);
	}

	bool FSteamAudioEditorModule::IsReadyToExportScene() const
	{
		return true;
	}

	bool FSteamAudioEditorModule::IsReadyToBakeReverb() const
	{
		return !GIsBakingReverb.load();
	}
}