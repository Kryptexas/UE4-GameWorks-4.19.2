// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "Runtime/Engine/Public/ShowFlags.h"
#include "SceneCaptureDetails.h"

#define LOCTEXT_NAMESPACE "SceneCaptureDetails"

TSharedRef<IDetailCustomization> FSceneCaptureDetails::MakeInstance()
{
	return MakeShareable( new FSceneCaptureDetails );
}

// Used to sort the show flags inside of their categories in the order of the text that is actually displayed
inline static bool SortAlphabeticallyByLocalizedText(const FString& ip1, const FString& ip2)
{
	FText LocalizedText1;
	FEngineShowFlags::FindShowFlagDisplayName(ip1, LocalizedText1);

	FText LocalizedText2;
	FEngineShowFlags::FindShowFlagDisplayName(ip2, LocalizedText2);

	return LocalizedText1.ToString() < LocalizedText2.ToString();
}

void FSceneCaptureDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	// Add all the properties that are there by default
	// (These would get added by default anyway, but we want to add them first so what we add next comes later in the list)
	TArray<TSharedRef<IPropertyHandle>> SceneCaptureCategoryDefaultProperties;
	IDetailCategoryBuilder& SceneCaptureCategoryBuilder = DetailLayout.EditCategory("SceneCapture");
	SceneCaptureCategoryBuilder.GetDefaultProperties(SceneCaptureCategoryDefaultProperties);

	for (TSharedRef<IPropertyHandle> Handle : SceneCaptureCategoryDefaultProperties)
	{
		SceneCaptureCategoryBuilder.AddProperty(Handle);
	}

	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailLayout.GetDetailsView().GetSelectedObjects();

	for( int32 ObjectIndex = 0; ObjectIndex < SelectedObjects.Num(); ++ObjectIndex )
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if ( CurrentObject.IsValid() )
		{
			ASceneCapture2D* CurrentCaptureActor2D = Cast<ASceneCapture2D>(CurrentObject.Get());
			if (CurrentCaptureActor2D != nullptr)
			{
				SceneCaptureComponent = Cast<USceneCaptureComponent>(CurrentCaptureActor2D->GetCaptureComponent2D());
				break;
			}
			ASceneCaptureCube* CurrentCaptureActorCube = Cast<ASceneCaptureCube>(CurrentObject.Get());
			if (CurrentCaptureActorCube != nullptr)
			{
				SceneCaptureComponent = Cast<USceneCaptureComponent>(CurrentCaptureActorCube->GetCaptureComponentCube());
				break;
			}
		}
	}

	// Hide certain categories
	TArray<EShowFlagGroup> ShowFlagGroupsToHideForCaptures;
	ShowFlagGroupsToHideForCaptures.Add(SFG_Hidden);
	ShowFlagGroupsToHideForCaptures.Add(SFG_CollisionModes);
	ShowFlagGroupsToHideForCaptures.Add(SFG_Visualize);
	ShowFlagGroupsToHideForCaptures.Add(SFG_Developer);
	ShowFlagGroupsToHideForCaptures.Add(SFG_PostProcess);

	// Hide certain flags
	TArray<FEngineShowFlags::EShowFlag> ShowFlagsToHideForCaptures;

	// General
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_AntiAliasing);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Collision);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Grid);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Navigation);

	// Advanced
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_AudioRadius);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_StreamingBounds);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Bounds);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_BSPSplit);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_CameraAspectRatioBars);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_CameraFrustums);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_CameraSafeFrames);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Constraints);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_HighResScreenshotMask);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_LevelColoration);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_LOD);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_MeshEdges);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_ModeWidgets);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_SeparateTranslucency);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_ShadowFrustums);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Splines);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_TemporalAA);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_VertexColors);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_VisualizeSenses);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Volumes);
	
	// Post process
	//ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_HMDDistortion);
	//ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_MotionBlur);

	// Lighting features
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_SubsurfaceScattering);

	// Lighting Components
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Diffuse);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_DirectLighting);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_DirectionalLights);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_PointLights);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_Specular);
	ShowFlagsToHideForCaptures.Add(FEngineShowFlags::EShowFlag::SF_SpotLights);
	
	// Create array of flag name strings for each group
	TArray< TArray<FString> > ShowFlagsByGroup;
	for (int32 GroupIndex = 0; GroupIndex < SFG_Max; ++GroupIndex)
	{
		ShowFlagsByGroup.Add(TArray<FString>());
	}

	// Enumerate all the flags and put them in the correct group
	bool FlagExists;
	int32 FlagIndex = 0;
	do 
	{
		TArray<FString> GroupStrings;
		FString FlagName;
		FlagName = FEngineShowFlags::FindNameByIndex(FlagIndex);
		FlagExists = !FlagName.IsEmpty();
		if (FlagExists)
		{
			EShowFlagGroup Group = FEngineShowFlags::FindShowFlagGroup(*FlagName);
			ShowFlagsByGroup[Group].Add(FlagName);
		}
		++FlagIndex;
	} while (FlagExists);

	// Sort the flags in their respective group alphabetically
	for (TArray<FString>& ShowFlagGroup : ShowFlagsByGroup)
	{
		ShowFlagGroup.Sort(SortAlphabeticallyByLocalizedText);
	}

	// Add each group
	for (int32 GroupIndex = 0; GroupIndex < SFG_Max; ++GroupIndex)
	{
		bool bGroupHidden = false;

		// Don't add show flag groups that we've specified to hide for Scene Captures
		for (EShowFlagGroup HiddenGroup : ShowFlagGroupsToHideForCaptures)
		{
			if (EShowFlagGroup(GroupIndex) == HiddenGroup)
			{
				bGroupHidden = true;
				break;
			}
		}

		if (!bGroupHidden)
		{
			FText GroupName;
			FText GroupTooltip;
			switch (GroupIndex)
			{
			case SFG_Normal:
				GroupName = LOCTEXT("CommonShowFlagHeader", "General Show Flags");
				break;
			case SFG_Advanced:
				GroupName = LOCTEXT("AdvancedShowFlagsMenu", "Advanced Show Flags");
				break;
			case SFG_PostProcess:
				GroupName = LOCTEXT("PostProcessShowFlagsMenu", "Post Processing Show Flags");
				break;
			case SFG_Developer:
				GroupName = LOCTEXT("DeveloperShowFlagsMenu", "Developer Show Flags");
				break;
			case SFG_Visualize:
				GroupName = LOCTEXT("VisualizeShowFlagsMenu", "Visualize Show Flags");
				break;
			case SFG_LightingComponents:
				GroupName = LOCTEXT("LightingComponentsShowFlagsMenu", "Lighting Components Show Flags");
				break;
			case SFG_LightingFeatures:
				GroupName = LOCTEXT("LightingFeaturesShowFlagsMenu", "Lighting Features Show Flags");
				break;
			case SFG_CollisionModes:
				GroupName = LOCTEXT("CollisionModesShowFlagsMenu", "Collision Modes Show Flags");
				break;
			case SFG_Hidden:
				GroupName = LOCTEXT("HiddenShowFlagsMenu", "Hidden Show Flags");
				break;
			default:
				// Should not get here unless a new group is added without being updated here
				GroupName = LOCTEXT("MiscFlagsMenu", "Misc Show Flags");
				break;
			}

			FName GroupFName = FName(*(GroupName.ToString()));
			IDetailGroup& Group = SceneCaptureCategoryBuilder.AddGroup(GroupFName, GroupName.ToString());

			// Add each show flag for this group
			for (FString& FlagName : ShowFlagsByGroup[GroupIndex])
			{
				bool bFlagHidden = false;
				FText LocalizedText;
				FEngineShowFlags::FindShowFlagDisplayName(FlagName, LocalizedText);

				for (FEngineShowFlags::EShowFlag HiddenFlag : ShowFlagsToHideForCaptures)
				{
					// Don't add show flag that we've specified to hide for Scene Captures
					// or that cannot be toggled in the editor
					if (FEngineShowFlags::EShowFlag(FEngineShowFlags::FindIndexByName(*FlagName)) == HiddenFlag 
						|| !FEngineShowFlags::CanBeToggledInEditor(*FlagName)
						)
					{
						bFlagHidden = true;
						break;
					}
				}

				if (!bFlagHidden)
				{
					Group.AddWidgetRow()
						.IsEnabled(true)
						.NameContent()
						[
							SNew(STextBlock)
							.Text(LocalizedText)
						]
					.ValueContent()
						[
							SNew(SCheckBox)
							.OnCheckStateChanged(this, &FSceneCaptureDetails::OnShowFlagCheckStateChanged, FlagName)
							.IsChecked(this, &FSceneCaptureDetails::OnGetDisplayCheckState, FlagName)
						]
					.FilterString(FlagName);
				}
			}
		}
	}
}

ESlateCheckBoxState::Type FSceneCaptureDetails::OnGetDisplayCheckState(FString ShowFlagName) const
{
	bool IsChecked = false;
	FEngineShowFlagsSetting* FlagSetting;
	if (SceneCaptureComponent->GetSettingForShowFlag(ShowFlagName, &FlagSetting))
	{
		IsChecked = FlagSetting->Enabled;
	}
	else
	{
		int32 SettingIndex = SceneCaptureComponent->ShowFlags.FindIndexByName(*(ShowFlagName));
		if (SettingIndex != INDEX_NONE)
		{
			IsChecked = SceneCaptureComponent->ShowFlags.GetSingleFlag(SettingIndex);
		}
	}

	return IsChecked ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void FSceneCaptureDetails::OnShowFlagCheckStateChanged(ESlateCheckBoxState::Type InNewRadioState, FString FlagName)
{
	SceneCaptureComponent->Modify();
	FEngineShowFlagsSetting* FlagSetting;
	bool bNewCheckState = (InNewRadioState == ESlateCheckBoxState::Checked);

	// If setting exists, update it
	if (SceneCaptureComponent->GetSettingForShowFlag(FlagName, &FlagSetting))
	{
		FlagSetting->Enabled = bNewCheckState;
	}
	// Otherwise create a new setting
	else
	{
		FEngineShowFlagsSetting NewFlagSetting;
		NewFlagSetting.ShowFlagName = FlagName;
		NewFlagSetting.Enabled = bNewCheckState;
		SceneCaptureComponent->ShowFlagSettings.Add(NewFlagSetting);
	}

	// Ensure PostEditChangeProperty is called, which in turn calls update
	SceneCaptureComponent->PostEditChange();
}

#undef LOCTEXT_NAMESPACE
