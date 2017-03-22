//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononSourceComponentDetails.h"

#include "PhononCommon.h"
#include "PhononSourceComponent.h"
#include "PhononProbeVolume.h"
#include "PhononSettings.h"
#include "PhononEditorModule.h"
#include "TickableNotification.h"
#include "phonon.h"
#include "Components/AudioComponent.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "IDetailChildrenBuilder.h"
#include "LevelEditorViewport.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
#include "EngineUtils.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#include "Editor.h"

namespace Phonon
{
	FPhononSourceComponentDetails::FPhononSourceComponentDetails()
		: BakePropagationTickable(MakeShareable(new FTickableNotification()))
	{
	}

	TSharedRef<IDetailCustomization> FPhononSourceComponentDetails::MakeInstance()
	{
		return MakeShareable(new FPhononSourceComponentDetails());
	}

	void FPhononSourceComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
	{
		const TArray<TWeakObjectPtr<UObject>>& SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();

		for (int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
		{
			const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
			if (CurrentObject.IsValid())
			{
				UPhononSourceComponent* SelectedPhononSourceComponent = Cast<UPhononSourceComponent>(CurrentObject.Get());
				if (SelectedPhononSourceComponent)
				{
					PhononSourceComponent = SelectedPhononSourceComponent;
					break;
				}
			}
		}

		DetailLayout.EditCategory("Baking")
			.AddCustomRow(NSLOCTEXT("PhononSourceComponentDetails", "Bake Propagation", "Bake Propagation"))
			.NameContent()
			[
				SNullWidget::NullWidget
			]
			.ValueContent()
			[
				SNew(SBox)
				.WidthOverride(125)
				[
					SNew(SButton)
					.ContentPadding(3)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.OnClicked(this, &FPhononSourceComponentDetails::OnBakePropagation)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("PhononSourceComponentDetails", "Bake Propagation", "Bake Propagation"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
					]
				]
			];
	}

	static void LoadSceneProgressCallback(float Progress)
	{

	}

	static void BakeProgressCallback(float Progress)
	{

	}

	static uint32 GeneratePhononSourceID(UWorld* World)
	{
		TMap<uint32, bool> SourceIDMap;

		for (TActorIterator<AActor> AActorItr(World); AActorItr; ++AActorItr)
		{
			if (AActorItr->GetComponentByClass(UPhononSourceComponent::StaticClass()) &&
				AActorItr->GetComponentByClass(UAudioComponent::StaticClass()))
			{
				UAudioComponent* AudioComponent = static_cast<UAudioComponent*>(AActorItr->GetComponentByClass(UAudioComponent::StaticClass()));
				SourceIDMap.Add(AudioComponent->GetAudioComponentUserID(), true);
			}
		}

		int32 NewSourceID = FMath::Rand();
		while (SourceIDMap.Contains(NewSourceID))
		{
			NewSourceID = FMath::Rand();
		}

		return NewSourceID;
	}

	FReply FPhononSourceComponentDetails::OnBakePropagation()
	{
		BakePropagationTickable->SetDisplayText(NSLOCTEXT("Phonon", "Baking propagation...", "Baking propagation..."));
		BakePropagationTickable->CreateNotification();

		UWorld* World = GEditor->LevelViewportClients[0]->GetWorld();
		check(World);

		// Get all probe volumes (cannot do this in the async task - not on game thread)
		TArray<AActor*> PhononProbeVolumes;
		UGameplayStatics::GetAllActorsOfClass(World, APhononProbeVolume::StaticClass(), PhononProbeVolumes);

		// Assign the audio component an ID if it doesn't have one
		{
			UAudioComponent* AudioComponent = static_cast<UAudioComponent*>(PhononSourceComponent->GetOwner()->GetComponentByClass(UAudioComponent::StaticClass()));
			if (!AudioComponent->GetAudioComponentUserID())
			{
				AudioComponent->AudioComponentUserID = GeneratePhononSourceID(World);
			}
		}

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
			IPLhandle PhononScene = nullptr;
			IPLhandle PhononEnvironment = nullptr;

			BakePropagationTickable->SetDisplayText(NSLOCTEXT("Phonon", "Loading scene...", "Loading scene..."));

			FString SceneFile = FPaths::GameDir() + "Content/" + World->GetMapName() + ".phononscene";

			if (FPaths::FileExists(SceneFile))
			{
				iplLoadFinalizedScene(Phonon::GlobalContext, SimulationSettings, TCHAR_TO_ANSI(*SceneFile), ComputeDevice, LoadSceneProgressCallback, &PhononScene);
				iplCreateEnvironment(Phonon::GlobalContext, ComputeDevice, SimulationSettings, PhononScene, nullptr, &PhononEnvironment);
			}
			else
			{
				UE_LOG(LogPhononEditor, Log, TEXT("Unable to load scene: %s not found."), *SceneFile);
			}

			BakePropagationTickable->SetDisplayText(NSLOCTEXT("Phonon", "Baking...", "Baking..."));

			for (AActor* PhononProbeVolumeActor : PhononProbeVolumes)
			{ 
				APhononProbeVolume* PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);
				
				IPLhandle ProbeBox = nullptr;
				iplLoadProbeBox(PhononProbeVolume->GetProbeBoxData(), PhononProbeVolume->GetProbeBoxDataSize(), &ProbeBox);
				
				IPLSphere SourceInfluence;
				SourceInfluence.radius = PhononSourceComponent->BakeRadius;
				SourceInfluence.center = Phonon::UnrealToPhononIPLVector3(PhononSourceComponent->GetComponentLocation());
				
				UAudioComponent* AudioComponent = static_cast<UAudioComponent*>(PhononSourceComponent->GetOwner()->GetComponentByClass(UAudioComponent::StaticClass()));
				FString AudioComponentName = FString::FromInt(AudioComponent->GetAudioComponentUserID());
				
				iplDeleteBakedDataByName(ProbeBox, TCHAR_TO_ANSI(*AudioComponentName));
				iplBakePropagation(PhononEnvironment, ProbeBox, SourceInfluence, TCHAR_TO_ANSI(*AudioComponentName), BakingSettings,
					BakeProgressCallback);

				PhononProbeVolume->UpdateProbeBoxData(ProbeBox);
				iplDestroyProbeBox(&ProbeBox);
			}

			iplDestroyEnvironment(&PhononEnvironment);
			iplDestroyScene(&PhononScene);

			// Display editor notification
			BakePropagationTickable->SetDisplayText(NSLOCTEXT("Phonon", "Bake complete!", "Bake complete!"));
			BakePropagationTickable->DestroyNotification();
		};

		TFuture<void> BakeResult = Async<void>(EAsyncExecution::Thread, BakeTask);
		return FReply::Handled();
	}
}