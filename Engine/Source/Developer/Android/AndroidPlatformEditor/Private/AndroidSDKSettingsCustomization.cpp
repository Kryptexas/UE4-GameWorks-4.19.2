// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AndroidPlatformEditorPrivatePCH.h"
#include "AndroidSDKSettingsCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "PropertyEditing.h"

#include "ScopedTransaction.h"
#include "SExternalImageReference.h"
#include "SHyperlinkLaunchURL.h"
#include "SPlatformSetupMessage.h"
#include "SFilePathPicker.h"
#include "PlatformIconInfo.h"
#include "SourceControlHelpers.h"
#include "ManifestUpdateHelper.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "ITargetPlatformManagerModule.h"

#define LOCTEXT_NAMESPACE "AndroidSDKSettings"

//////////////////////////////////////////////////////////////////////////
// FAndroidSDKSettingsCustomization

TSharedRef<IDetailCustomization> FAndroidSDKSettingsCustomization::MakeInstance()
{
	return MakeShareable(new FAndroidSDKSettingsCustomization);
}

FAndroidSDKSettingsCustomization::FAndroidSDKSettingsCustomization()
{
	TargetPlatformManagerModule = &FModuleManager::LoadModuleChecked<ITargetPlatformManagerModule>("TargetPlatform");
	SetupSDKPaths();
}

void FAndroidSDKSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	SavedLayoutBuilder = &DetailLayout;
	BuildSDKPathSection(DetailLayout);
}


void FAndroidSDKSettingsCustomization::BuildSDKPathSection(IDetailLayoutBuilder& DetailLayout)
{
	SetupSDKPaths();}


void FAndroidSDKSettingsCustomization::SetupSDKPaths()
{
	// Try and query the env vars
	UAndroidSDKSettings * settings = GetMutableDefault<UAndroidSDKSettings>();
	bool changed = false;
	
	if (settings->SDKPath.Path.IsEmpty())
	{
		TCHAR AndroidSDKPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("ANDROID_HOME"), AndroidSDKPath, ARRAY_COUNT(AndroidSDKPath));
		if (AndroidSDKPath[0])
		{
			settings->SDKPath.Path = AndroidSDKPath;
			changed |= true;
		}
				
	}
	

	if (settings->NDKPath.Path.IsEmpty())
	{
		TCHAR AndroidNDKPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("NDKROOT"), AndroidNDKPath, ARRAY_COUNT(AndroidNDKPath));
		if (AndroidNDKPath[0])
		{
			settings->NDKPath.Path = AndroidNDKPath;
			changed |= true;
		}
		
	}
	

	if (settings->ANTPath.Path.IsEmpty())
	{
		TCHAR AndroidANTPath[256];
		FPlatformMisc::GetEnvironmentVariable(TEXT("ANT_HOME"), AndroidANTPath, ARRAY_COUNT(AndroidANTPath));
		if (AndroidANTPath[0])
		{
			settings->ANTPath.Path = AndroidANTPath;
			changed |= true;
		}
		
	}
	
	if (changed)
	{
		settings->Modify();
		settings->SaveConfig();
	}

	settings->UpdateTargetModulePaths(false);

}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
