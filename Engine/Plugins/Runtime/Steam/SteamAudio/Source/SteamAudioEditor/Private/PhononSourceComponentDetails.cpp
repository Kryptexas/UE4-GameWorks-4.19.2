//
// Copyright (C) Valve Corporation. All rights reserved.
//

#include "PhononSourceComponentDetails.h"

#include "PhononCommon.h"
#include "PhononSourceComponent.h"
#include "PhononProbeVolume.h"
#include "SteamAudioSettings.h"
#include "SteamAudioEditorModule.h"
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
#include "Widgets/Text/STextBlock.h"
#include "Editor.h"

#include <atomic>

namespace SteamAudio
{
	//==============================================================================================================================================
	// Global data and functions for baking
	//==============================================================================================================================================

	static TSharedPtr<SteamAudio::FTickableNotification> GBakeSourcePropagationTickable = MakeShareable(new SteamAudio::FTickableNotification());
	static int32 GCurrentSourceProbeVolume = 0;
	static int32 GNumSourceProbeVolumes = 0;
	static std::atomic<bool> GIsBakingSourcePropagation = false;

	static void LoadSceneProgressCallback(float Progress)
	{
	}

	static void BakePropagationProgressCallback(float Progress)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("BakeProgress"), FText::AsPercent(Progress));
		Arguments.Add(TEXT("CurrentProbeVolume"), FText::AsNumber(GCurrentSourceProbeVolume));
		Arguments.Add(TEXT("NumProbeVolumes"), FText::AsNumber(GNumSourceProbeVolumes));
		GBakeSourcePropagationTickable->SetDisplayText(FText::Format(NSLOCTEXT("SteamAudio", "BakeText",
			"Baking {CurrentProbeVolume}/{NumProbeVolumes} probe volumes ({BakeProgress} complete)"), Arguments));
	}

	//==============================================================================================================================================
	// FPhononSourceComponentDetails
	//==============================================================================================================================================

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
				auto SelectedPhononSourceComponent = Cast<UPhononSourceComponent>(CurrentObject.Get());
				if (SelectedPhononSourceComponent)
				{
					PhononSourceComponent = SelectedPhononSourceComponent;
					break;
				}
			}
		}

		DetailLayout.EditCategory("Baking").AddProperty(GET_MEMBER_NAME_CHECKED(UPhononSourceComponent, UniqueIdentifier));
		DetailLayout.EditCategory("Baking").AddProperty(GET_MEMBER_NAME_CHECKED(UPhononSourceComponent, BakingRadius));
		DetailLayout.EditCategory("Baking").AddCustomRow(NSLOCTEXT("PhononSourceComponentDetails", "Bake Propagation", "Bake Propagation"))
			.NameContent()
			[
				SNullWidget::NullWidget
			]
			.ValueContent()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ContentPadding(2)
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
					.IsEnabled(this, &FPhononSourceComponentDetails::IsBakeEnabled)
					.OnClicked(this, &FPhononSourceComponentDetails::OnBakePropagation)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("PhononSourceComponentDetails", "Bake Propagation", "Bake Propagation"))
						.Font(IDetailLayoutBuilder::GetDetailFont())
					]
				]
			];
	}

	FReply FPhononSourceComponentDetails::OnBakePropagation()
	{
		GIsBakingSourcePropagation.store(true);

		GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Baking propagation...", "Baking propagation..."));
		GBakeSourcePropagationTickable->CreateNotificationWithCancel(FSimpleDelegate::CreateRaw(this, &FPhononSourceComponentDetails::CancelBakePropagation));

		auto World = GEditor->LevelViewportClients[0]->GetWorld();
		check(World);

		// Get all probe volumes (cannot do this in the async task - not on game thread)
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
			GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake failed.", "Bake failed. Create at least one Phonon Probe Volume that has probes."));
			GBakeSourcePropagationTickable->DestroyNotification(SNotificationItem::CS_Fail);
			GIsBakingSourcePropagation.store(false);
			return FReply::Handled();
		}

		// Set the User ID on the audio component
		auto AudioComponent = static_cast<UAudioComponent*>(PhononSourceComponent->GetOwner()->GetComponentByClass(UAudioComponent::StaticClass()));
		AudioComponent->AudioComponentUserID = PhononSourceComponent->UniqueIdentifier;

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
			IPLhandle PhononScene = nullptr;
			IPLhandle PhononEnvironment = nullptr;

			// Grab a copy of the component ptr because it will be destroyed if user clicks off of it in GUI
			auto PhononSourceComponentHandle = PhononSourceComponent.Get();

			GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Loading scene...", "Loading scene..."));

			FString SceneFile = FPaths::GameDir() + "Content/" + World->GetMapName() + ".phononscene";

			if (FPaths::FileExists(SceneFile))
			{
				iplLoadFinalizedScene(SteamAudio::GlobalContext, SimulationSettings, TCHAR_TO_ANSI(*SceneFile), ComputeDevice, LoadSceneProgressCallback, &PhononScene);
				iplCreateEnvironment(SteamAudio::GlobalContext, ComputeDevice, SimulationSettings, PhononScene, nullptr, &PhononEnvironment);
			}
			else
			{
				UE_LOG(LogSteamAudioEditor, Error, TEXT("Unable to load scene: %s not found."), *SceneFile);
				GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake failed.", "Bake failed. Export scene first."));
				GBakeSourcePropagationTickable->DestroyNotification(SNotificationItem::CS_Fail);
				GIsBakingSourcePropagation.store(false);
				return;
			}

			GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Baking...", "Baking..."));
			GNumSourceProbeVolumes = PhononProbeVolumes.Num();
			GCurrentSourceProbeVolume = 1;

			for (auto PhononProbeVolumeActor : PhononProbeVolumes)
			{
				auto PhononProbeVolume = Cast<APhononProbeVolume>(PhononProbeVolumeActor);

				IPLhandle ProbeBox = nullptr;
				iplLoadProbeBox(PhononProbeVolume->GetProbeBoxData(), PhononProbeVolume->GetProbeBoxDataSize(), &ProbeBox);

				IPLSphere SourceInfluence;
				SourceInfluence.radius = PhononSourceComponentHandle->BakingRadius * SteamAudio::SCALEFACTOR;
				SourceInfluence.center = SteamAudio::UnrealToPhononIPLVector3(PhononSourceComponentHandle->GetComponentLocation());

				auto AudioComponent = static_cast<UAudioComponent*>(PhononSourceComponentHandle->GetOwner()->GetComponentByClass(UAudioComponent::StaticClass()));

				iplDeleteBakedDataByName(ProbeBox, TCHAR_TO_ANSI(*AudioComponent->GetAudioComponentUserID().ToString()));
				iplBakePropagation(PhononEnvironment, ProbeBox, SourceInfluence, TCHAR_TO_ANSI(*AudioComponent->GetAudioComponentUserID().ToString()),
					BakingSettings, BakePropagationProgressCallback);

				FBakedDataInfo BakedDataInfo;
				BakedDataInfo.Name = PhononSourceComponentHandle->UniqueIdentifier;
				BakedDataInfo.Size = iplGetBakedDataSizeByName(ProbeBox, TCHAR_TO_ANSI(*PhononSourceComponentHandle->UniqueIdentifier.ToString()));
				
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

				if (!GIsBakingSourcePropagation.load())
				{
					iplDestroyEnvironment(&PhononEnvironment);
					iplDestroyScene(&PhononScene);
					GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake propagation cancelled.", "Bake propagation cancelled."));
					GBakeSourcePropagationTickable->DestroyNotification(SNotificationItem::CS_Fail);
					return;
				}

				PhononProbeVolume->UpdateProbeBoxData(ProbeBox);
				iplDestroyProbeBox(&ProbeBox);
				++GCurrentSourceProbeVolume;
			}

			iplDestroyEnvironment(&PhononEnvironment);
			iplDestroyScene(&PhononScene);

			// Display editor notification
			GBakeSourcePropagationTickable->SetDisplayText(NSLOCTEXT("SteamAudio", "Bake propagation complete.", "Bake propagation complete."));
			GBakeSourcePropagationTickable->DestroyNotification();
			GIsBakingSourcePropagation.store(false);
		});

		return FReply::Handled();
	}

	void FPhononSourceComponentDetails::CancelBakePropagation()
	{
		iplCancelBake();
		GIsBakingSourcePropagation.store(false);
	}

	bool FPhononSourceComponentDetails::IsBakeEnabled() const
	{
		return !(PhononSourceComponent->UniqueIdentifier.IsNone() || GIsBakingSourcePropagation.load());
	}
}