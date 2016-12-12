// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AssetViewerSettings.h"
#include "UObject/UnrealType.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Editor.h"


UAssetViewerSettings::UAssetViewerSettings()
{
	// Make sure there always is one profile as default
	Profiles.AddDefaulted(1);
	Profiles[0].ProfileName = TEXT("Profile_0");
	NumProfiles = 1;
}

UAssetViewerSettings::~UAssetViewerSettings()
{
	if (GEditor)
	{
		GEditor->UnregisterForUndo(this);
	}
}

UAssetViewerSettings* UAssetViewerSettings::Get()
{
	// This is a singleton, use default object
	UAssetViewerSettings* DefaultSettings = GetMutableDefault<UAssetViewerSettings>();

	// Load environment map textures (once)
	static bool bInitialized = false;
	if (!bInitialized)
	{
		DefaultSettings->SetFlags(RF_Transactional);

		for (FPreviewSceneProfile& Profile : DefaultSettings->Profiles)
		{
			Profile.LoadEnvironmentMap();
		}

		bInitialized = true;

		if (GEditor)
		{
			GEditor->RegisterForUndo(DefaultSettings);
		}
	}

	return DefaultSettings;
}

void UAssetViewerSettings::Save()
{
	SaveConfig();
	UpdateDefaultConfigFile();
}

void UAssetViewerSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;	
	UObject* Outer = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetOuter() : nullptr;
	if (Outer != nullptr && ( Outer->GetName() == "PostProcessSettings" || Outer->GetName() == "Vector" || Outer->GetName() == "Vector4" || Outer->GetName() == "LinearColor"))
	{
		PropertyName = GET_MEMBER_NAME_CHECKED(FPreviewSceneProfile, PostProcessingSettings);
	}

	// Store path to the set enviroment map texture
	if (PropertyName == GET_MEMBER_NAME_CHECKED(FPreviewSceneProfile, EnvironmentCubeMap))
	{
		int32 ProfileIndex = GetMutableDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex;
		Profiles[ProfileIndex].EnvironmentCubeMapPath = Profiles[ProfileIndex].EnvironmentCubeMap.ToString();
	}

	if (NumProfiles != Profiles.Num())
	{
		OnAssetViewerProfileAddRemovedEvent.Broadcast();
		NumProfiles = Profiles.Num();
	}
	
	OnAssetViewerSettingsChangedEvent.Broadcast(PropertyName);
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

void UAssetViewerSettings::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
		OnAssetViewerSettingsPostUndoEvent.Broadcast();
	}
}

void UAssetViewerSettings::PostRedo(bool bSuccess)
{
	PostUndo(bSuccess);
}
