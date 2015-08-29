// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "InternationalizationSettingsModulePrivatePCH.h"

/* UInternationalizationSettingsModel interface
 *****************************************************************************/

UInternationalizationSettingsModel::UInternationalizationSettingsModel( const FObjectInitializer& ObjectInitializer )
	: Super(ObjectInitializer)
{ }

void UInternationalizationSettingsModel::SaveDefaults()
{
	FString SavedCultureName;
	GConfig->GetString( TEXT("Internationalization"), TEXT("Culture"), SavedCultureName, GEditorSettingsIni );
	GConfig->SetString( TEXT("Internationalization"), TEXT("Culture"), *SavedCultureName, GEngineIni );

	bool bShouldLoadLocalizedPropertyNames = true;
	GConfig->GetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), bShouldLoadLocalizedPropertyNames, GEditorSettingsIni );
	GConfig->SetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), bShouldLoadLocalizedPropertyNames, GEngineIni );

	bool bShowNodesAndPinsUnlocalized = false;
	GConfig->GetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), bShowNodesAndPinsUnlocalized, GEditorSettingsIni );
	GConfig->SetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), bShowNodesAndPinsUnlocalized, GEngineIni );
}

void UInternationalizationSettingsModel::ResetToDefault()
{
	FString SavedCultureName;
	GConfig->GetString( TEXT("Internationalization"), TEXT("Culture"), SavedCultureName, GEngineIni );
	GConfig->SetString( TEXT("Internationalization"), TEXT("Culture"), *SavedCultureName, GEditorSettingsIni );

	GConfig->SetString( TEXT("Internationalization"), TEXT("NativeGameCulture"), TEXT(""), GEditorSettingsIni );

	bool bShouldLoadLocalizedPropertyNames = true;
	GConfig->GetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), bShouldLoadLocalizedPropertyNames, GEngineIni );
	GConfig->SetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), bShouldLoadLocalizedPropertyNames, GEditorSettingsIni );

	bool bShowNodesAndPinsUnlocalized = false;
	GConfig->GetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), bShowNodesAndPinsUnlocalized, GEngineIni );
	GConfig->SetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), bShowNodesAndPinsUnlocalized, GEditorSettingsIni );

	SettingsChangedEvent.Broadcast();
}

FString UInternationalizationSettingsModel::GetEditorCultureName() const
{
	FString SavedCultureName;
	if( !GConfig->GetString( TEXT("Internationalization"), TEXT("Culture"), SavedCultureName, GEditorSettingsIni ) )
	{
		GConfig->GetString( TEXT("Internationalization"), TEXT("Culture"), SavedCultureName, GEngineIni );
	}
	return SavedCultureName;
}

void UInternationalizationSettingsModel::SetEditorCultureName(const FString& CultureName)
{
	GConfig->SetString( TEXT("Internationalization"), TEXT("Culture"), *CultureName, GEditorSettingsIni );
	SettingsChangedEvent.Broadcast();
}

FString UInternationalizationSettingsModel::GetNativeGameCultureName() const
{
	FString SavedCultureName;
	GConfig->GetString( TEXT("Internationalization"), TEXT("NativeGameCulture"), SavedCultureName, GEditorSettingsIni );
	return SavedCultureName;
}

void UInternationalizationSettingsModel::SetNativeGameCultureName(const FString& CultureName)
{
	GConfig->SetString( TEXT("Internationalization"), TEXT("NativeGameCulture"), *CultureName, GEditorSettingsIni );
	SettingsChangedEvent.Broadcast();
}

bool UInternationalizationSettingsModel::ShouldLoadLocalizedPropertyNames() const
{
	bool bShouldLoadLocalizedPropertyNames = true;
	if( !GConfig->GetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), bShouldLoadLocalizedPropertyNames, GEditorSettingsIni ) )
	{
		GConfig->GetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), bShouldLoadLocalizedPropertyNames, GEngineIni );
	}
	return bShouldLoadLocalizedPropertyNames;
}

void UInternationalizationSettingsModel::ShouldLoadLocalizedPropertyNames(const bool Value)
{
	GConfig->SetBool( TEXT("Internationalization"), TEXT("ShouldLoadLocalizedPropertyNames"), Value, GEditorSettingsIni );
	SettingsChangedEvent.Broadcast();
}

bool UInternationalizationSettingsModel::ShouldShowNodesAndPinsUnlocalized() const
{
	bool bShowNodesAndPinsUnlocalized = false;
	if( !GConfig->GetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), bShowNodesAndPinsUnlocalized, GEditorSettingsIni ) )
	{
		GConfig->GetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), bShowNodesAndPinsUnlocalized, GEngineIni );
	}
	return bShowNodesAndPinsUnlocalized;
}

void UInternationalizationSettingsModel::ShouldShowNodesAndPinsUnlocalized(const bool Value)
{
	GConfig->SetBool( TEXT("Internationalization"), TEXT("ShowNodesAndPinsUnlocalized"), Value, GEditorSettingsIni );
	SettingsChangedEvent.Broadcast();
}
