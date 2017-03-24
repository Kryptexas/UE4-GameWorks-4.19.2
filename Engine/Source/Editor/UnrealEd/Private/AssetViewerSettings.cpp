// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AssetViewerSettings.h"
#include "UObject/UnrealType.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Editor.h"


UAssetViewerSettings::UAssetViewerSettings()
{
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
	LocalProfiles.Empty();
	SharedProfiles.Empty();

	// Divide profiles up in corresponding arrays
	for (FPreviewSceneProfile& Profile : Profiles)
	{
		if (Profile.bSharedProfile)
		{
			SharedProfiles.Add(Profile);
		}
		else
		{
			LocalProfiles.Add(Profile);
		}
	}

	UClass* Class = GetClass();
	UField* Child = Class->Children;
	const FString LocalFilename = GetConfigFilename(this);
	const FString SharedFilename = GetDefaultConfigFilename();	

	// Store name for last selected profile
	GetMutableDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileName = Profiles[GetMutableDefault<UEditorPerProjectUserSettings>()->AssetViewerProfileIndex].ProfileName;

	UProperty* PropertyLink = Class->PropertyLink;
	for (UProperty* Property = GetClass()->PropertyLink; Property; Property = Property->PropertyLinkNext)
	{
		const FName PropertyName = Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UAssetViewerSettings, LocalProfiles))
		{
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
			UpdateSinglePropertyInConfigFile(ArrayProperty->Inner, LocalFilename);
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(UAssetViewerSettings, SharedProfiles))
		{
			UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
			UpdateSinglePropertyInConfigFile(ArrayProperty->Inner, SharedFilename);
		}
	}
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

	Profiles.Empty();
	for (FPreviewSceneProfile& Profile : SharedProfiles)
	{
		Profiles.Add(Profile);
	}

	for (FPreviewSceneProfile& Profile : LocalProfiles)
	{
		Profiles.Add(Profile);
	}

	if (Profiles.Num() == 0)
	{
		// Make sure there always is one profile as default
		Profiles.AddDefaulted(1);
		Profiles[0].ProfileName = TEXT("Profile_0");
	}
	NumProfiles = Profiles.Num();

	UEditorPerProjectUserSettings* ProjectSettings = GetMutableDefault<UEditorPerProjectUserSettings>();

	// Find last saved profile index
	int32 SelectedProfileIndex = INDEX_NONE;
	for (int32 ProfileIndex = 0; ProfileIndex < Profiles.Num(); ++ProfileIndex)
	{
		if (Profiles[ProfileIndex].ProfileName == ProjectSettings->AssetViewerProfileName)
		{
			SelectedProfileIndex = ProfileIndex;
			break;
		}
	}

	// Only need to set profile index if we found a correct value since it defaults to 0
	if (SelectedProfileIndex != INDEX_NONE)
	{
		ProjectSettings->AssetViewerProfileIndex = SelectedProfileIndex;
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
