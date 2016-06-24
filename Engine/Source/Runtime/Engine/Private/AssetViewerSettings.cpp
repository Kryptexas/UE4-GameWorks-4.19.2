// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AssetViewerSettings.h"

#if WITH_EDITOR
#include "Editor/EditorPerProjectUserSettings.h"
#endif // WITH_EDITOR

UAssetViewerSettings::UAssetViewerSettings()
{
	// Make sure there always is one profile as default
	Profiles.AddDefaulted(1);
	Profiles[0].ProfileName = TEXT("Profile_0");

#if WITH_EDITOR
	NumProfiles = Profiles.Num();
#endif // WITH_EDITOR
}

UAssetViewerSettings* UAssetViewerSettings::Get()
{
	// This is a singleton, use default object
	UAssetViewerSettings* DefaultSettings = GetMutableDefault<UAssetViewerSettings>();	
	return DefaultSettings;
}

#if WITH_EDITOR
void UAssetViewerSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;	
	UObject* Outer = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetOuter() : nullptr;
	if (Outer != nullptr && Outer->GetName() == "PostProcessSettings")
	{
		PropertyName = GET_MEMBER_NAME_CHECKED(FPreviewSceneProfile, PostProcessingSettings);
	}

	if (NumProfiles != Profiles.Num())
	{
		OnAssetViewerProfileAddRemovedEvent.Broadcast();
		NumProfiles = Profiles.Num();
	}
	
	OnAssetViewerSettingsChangedEvent.Broadcast(PropertyName);

	SaveConfig();
	UpdateDefaultConfigFile();
}

void UAssetViewerSettings::PostInitProperties()
{
	Super::PostInitProperties();
	NumProfiles = Profiles.Num();

	UEditorPerProjectUserSettings* ProjectSettings = GetMutableDefault<UEditorPerProjectUserSettings>();
	if (!Profiles.IsValidIndex(ProjectSettings->AssetViewerProfileIndex))
	{
		ProjectSettings->AssetViewerProfileIndex = 0;
	}
}

#endif // WITH_EDITOR