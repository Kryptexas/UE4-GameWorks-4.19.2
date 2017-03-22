//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononEditorModule.h"

#include "ISettingsModule.h"

#include "Editor.h"

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
#include "MultiBox/MultiBoxBuilder.h"

#include "TickableNotification.h"
#include "PhononScene.h"
#include "PhononSettings.h"
#include "PhononProbeVolume.h"
#include "PhononProbeVolumeDetails.h"
#include "PhononSourceComponent.h"
#include "PhononSourceComponentDetails.h"
#include "PhononSourceComponentVisualizer.h"

DEFINE_LOG_CATEGORY(LogPhononEditor);

IMPLEMENT_MODULE(Phonon::FPhononEditorModule, PhononEditor)

namespace Phonon
{
	void FPhononEditorModule::StartupModule()
	{
		// Create tickable notifications
		ExportSceneTickable = MakeShareable(new FTickableNotification());
		BakeReverbTickable = MakeShareable(new FTickableNotification());

		// Register detail customizations
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.RegisterCustomClassLayout("PhononProbeVolume", FOnGetDetailCustomizationInstance::CreateStatic(&FPhononProbeVolumeDetails::MakeInstance));
		PropertyModule.RegisterCustomClassLayout("PhononSourceComponent",  FOnGetDetailCustomizationInstance::CreateStatic(&FPhononSourceComponentDetails::MakeInstance));

		// Register component visualizers
		RegisterComponentVisualizer(UPhononSourceComponent::StaticClass()->GetFName(), MakeShareable(new FPhononSourceComponentVisualizer()));

		// Extend the toolbar build menu with custom actions
		if (!IsRunningCommandlet())
		{
			FLevelEditorModule* LevelEditorModule = FModuleManager::LoadModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
			if (LevelEditorModule)
			{
				auto BuildMenuExtender = FLevelEditorModule::FLevelEditorMenuExtender::CreateRaw(this, &FPhononEditorModule::OnExtendLevelEditorBuildMenu);

				LevelEditorModule->GetAllLevelEditorToolbarBuildMenuExtenders().Add(BuildMenuExtender);
			}
		}

		// Register plugin settings
		ISettingsModule* SettingsModule = FModuleManager::Get().GetModulePtr<ISettingsModule>("Settings");
		if (SettingsModule)
		{
			SettingsModule->RegisterSettings("Project", "Plugins", "Steam Audio", NSLOCTEXT("Steam Audio", "Steam Audio", "Steam Audio"),
				NSLOCTEXT("Phonon", "Configure Steam Audio settings", "Configure Steam Audio settings"), GetMutableDefault<UPhononSettings>());
		}

		// Create and register custom slate style
		FString PhononContent = IPluginManager::Get().FindPlugin("Phonon")->GetBaseDir() + "/Content";
		FVector2D Vec16 = FVector2D(16.0f, 16.0f);

		PhononStyleSet = MakeShareable(new FSlateStyleSet("Phonon"));
		PhononStyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
		PhononStyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));
		PhononStyleSet->Set("ClassIcon.PhononSourceComponent", new FSlateImageBrush(PhononContent + "/S_PhononSource_16.png", Vec16));
		PhononStyleSet->Set("ClassIcon.PhononListenerComponent", new FSlateImageBrush(PhononContent + "/S_PhononListener_16.png", Vec16));
		PhononStyleSet->Set("ClassIcon.PhononGeometryComponent", new FSlateImageBrush(PhononContent + "/S_PhononGeometry_16.png", Vec16));
		PhononStyleSet->Set("ClassIcon.PhononMaterialComponent", new FSlateImageBrush(PhononContent + "/S_PhononMaterial_16.png", Vec16));
		FSlateStyleRegistry::RegisterSlateStyle(*PhononStyleSet.Get());
	}

	void FPhononEditorModule::ShutdownModule()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*PhononStyleSet.Get());

		if (GUnrealEd)
		{
			for (FName& ClassName : RegisteredComponentClassNames)
			{
				GUnrealEd->UnregisterComponentVisualizer(ClassName);
			}
		}
	}

	void FPhononEditorModule::RegisterComponentVisualizer(const FName ComponentClassName, TSharedPtr<FComponentVisualizer> Visualizer)
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

	TSharedRef<FExtender> FPhononEditorModule::OnExtendLevelEditorBuildMenu(const TSharedRef<FUICommandList> CommandList)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		Extender->AddMenuExtension("LevelEditorNavigation", EExtensionHook::After, nullptr, 
			FMenuExtensionDelegate::CreateRaw(this, &FPhononEditorModule::CreateBuildMenu));

		return Extender;
	}

	void FPhononEditorModule::CreateBuildMenu(FMenuBuilder& Builder)
	{
		FUIAction ActionExportScene(FExecuteAction::CreateRaw(this, &FPhononEditorModule::ExportScene),
			FCanExecuteAction::CreateRaw(this, &FPhononEditorModule::IsReadyToExportScene));

		FUIAction ActionBakeReverb(FExecuteAction::CreateRaw(this, &FPhononEditorModule::BakeReverb),
			FCanExecuteAction::CreateRaw(this, &FPhononEditorModule::IsReadyToBakeReverb));

		Builder.BeginSection("LevelEditorIR", NSLOCTEXT("Phonon", "Phonon", "Phonon"));

		Builder.AddMenuEntry(NSLOCTEXT("Phonon", "Export Scene", "Export Scene"),
			NSLOCTEXT("Phonon", "Exports Phonon geometry.", "Export Phonon geometry."), FSlateIcon(), ActionExportScene, NAME_None,
			EUserInterfaceActionType::Button);

		Builder.AddMenuEntry(NSLOCTEXT("Phonon", "Bake Reverb", "Bake Reverb"),
			NSLOCTEXT("Phonon", "Bakes reverb at all probe locations.", "Bakes reverb at all probe locations."), FSlateIcon(), ActionBakeReverb,
			NAME_None, EUserInterfaceActionType::Button);

		Builder.EndSection();
	}

	void FPhononEditorModule::ExportScene()
	{
		// Display editor notification
		ExportSceneTickable->SetDisplayText(NSLOCTEXT("Phonon", "Exporting scene...", "Exporting scene..."));
		ExportSceneTickable->CreateNotification();

		// Export the scene
		UWorld* World = GEditor->LevelViewportClients[0]->GetWorld();
		IPLhandle PhononScene = nullptr;
		TArray<IPLhandle> PhononStaticMeshes;
		Phonon::LoadScene(World, &PhononScene, &PhononStaticMeshes);

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
		ExportSceneTickable->SetDisplayText(NSLOCTEXT("Phonon", "Export successful!", "Export successful!"));
		ExportSceneTickable->DestroyNotification();
	}

	bool FPhononEditorModule::IsReadyToExportScene() const
	{
		return true;
	}

	void FPhononEditorModule::BakeReverb()
	{
		// Display editor notification
		BakeReverbTickable->SetDisplayText(NSLOCTEXT("Phonon", "Baking reverb...", "Baking reverb..."));
		BakeReverbTickable->CreateNotification();

		UWorld* World = GEditor->LevelViewportClients[0]->GetWorld();

		// Get all probe volumes (cannot do this in the async task)
		TArray<AActor*> PhononProbeVolumes;
		UGameplayStatics::GetAllActorsOfClass(World, APhononProbeVolume::StaticClass(), PhononProbeVolumes);

		TFunction<void()> BakeTask = [=]()
		{
			IPLBakingSettings BakingSettings;
			BakingSettings.bakeParametric = IPL_FALSE;
			BakingSettings.bakeConvolution = IPL_TRUE;

			IPLSimulationSettings SimulationSettings;
			SimulationSettings.sceneType = IPL_SCENETYPE_PHONON;
			SimulationSettings.irDuration = GetDefault<UPhononSettings>()->IndirectImpulseResponseDuration;
			SimulationSettings.ambisonicsOrder = GetDefault<UPhononSettings>()->IndirectImpulseResponseOrder;
			SimulationSettings.maxConvolutionSources = 0;
			SimulationSettings.numBounces = GetDefault<UPhononSettings>()->BakedBounces;
			SimulationSettings.numRays = GetDefault<UPhononSettings>()->BakedRays;
			SimulationSettings.numDiffuseSamples = GetDefault<UPhononSettings>()->BakedSecondaryRays;

			IPLhandle ComputeDevice = nullptr;
			//iplCreateComputeDevice(IPL_COMPUTEDEVICE_CPU, 1, &ComputeDevice);

			IPLLoadSceneProgressCallback LoadSceneProgressCallback = nullptr;

			IPLhandle PhononScene = nullptr;
			IPLhandle PhononEnvironment = nullptr;

			BakeReverbTickable->SetDisplayText(NSLOCTEXT("Phonon", "Loading scene...", "Loading scene..."));

			if (World)
			{
				FString SceneFile = FPaths::GameDir() + "Content/" + World->GetMapName() + ".phononscene";

				if (FPaths::FileExists(SceneFile))
				{
					iplLoadFinalizedScene(Phonon::GlobalContext, SimulationSettings, TCHAR_TO_ANSI(*SceneFile), ComputeDevice, LoadSceneProgressCallback, &PhononScene);
					iplCreateEnvironment(Phonon::GlobalContext, ComputeDevice, SimulationSettings, PhononScene, nullptr, &PhononEnvironment);
				}
				else
				{
					// TODO: load 
					UE_LOG(LogPhononEditor, Log, TEXT("Unable to load scene: %s not found."), *SceneFile);
				}
			}
			else
			{
				UE_LOG(LogPhononEditor, Log, TEXT("Unable to load scene: null World."));
			}

			BakeReverbTickable->SetDisplayText(NSLOCTEXT("Phonon", "Baking reverb...", "Baking reverb..."));

			for (AActor* PhononProbeVolumeActor : PhononProbeVolumes)
			{
				APhononProbeVolume* PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);
				IPLhandle ProbeBox = nullptr;
				iplLoadProbeBox(PhononProbeVolume->GetProbeBoxData(), PhononProbeVolume->GetProbeBoxDataSize(), &ProbeBox);
				iplBakeReverb(PhononEnvironment, ProbeBox, BakingSettings, nullptr);
				PhononProbeVolume->UpdateProbeBoxData(ProbeBox);
				iplDestroyProbeBox(&ProbeBox);
			}

			iplDestroyEnvironment(&PhononEnvironment);
			iplDestroyScene(&PhononScene);

			// Display editor notification
			BakeReverbTickable->SetDisplayText(NSLOCTEXT("Phonon", "Bake complete!", "Bake complete!"));
			BakeReverbTickable->DestroyNotification();
		};

		TFuture<void> BakeResult = Async<void>(EAsyncExecution::Thread, BakeTask);
	}

	bool FPhononEditorModule::IsReadyToBakeReverb() const
	{
		return true;
	}
}