// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

#include "Json.h"
#include "ModuleManager.h"

const FString& FLayoutSaveRestore::GetAdditionalLayoutConfigIni()
{
	static const FString IniSectionAdditionalConfig = TEXT("SlateAdditionalLayoutConfig");
	return IniSectionAdditionalConfig;
}


void FLayoutSaveRestore::SaveTheLayout( const TSharedRef<FTabManager::FLayout>& LayoutToSave )
{
	SaveToConfig(LayoutToSave, GEditorUserSettingsIni);
}

void FLayoutSaveRestore::SaveToConfig( const TSharedRef<FTabManager::FLayout>& LayoutToSave, FString ConfigFileName )
{
	const FString LayoutAsString = FLayoutSaveRestore::PrepareLayoutStringForIni( LayoutToSave->ToString() );

	GConfig->SetString( TEXT("EditorLayouts"), *LayoutToSave->GetLayoutName().ToString(), *LayoutAsString, ConfigFileName );
}

TSharedRef<FTabManager::FLayout> FLayoutSaveRestore::LoadUserConfigVersionOf( const TSharedRef<FTabManager::FLayout>& DefaultLayout )
{
	return LoadFromConfig(DefaultLayout,GEditorUserSettingsIni);
}

TSharedRef<FTabManager::FLayout> FLayoutSaveRestore::LoadFromConfig( const TSharedRef<FTabManager::FLayout>& DefaultLayout, FString ConfigFileName )
{
	const FName LayoutName = DefaultLayout->GetLayoutName();
	FString UserLayoutString;
	if ( GConfig->GetString( TEXT("EditorLayouts"), *LayoutName.ToString(), UserLayoutString, ConfigFileName ) && !UserLayoutString.IsEmpty() )
	{
		TSharedPtr<FTabManager::FLayout> UserLayout = FTabManager::FLayout::NewFromString( FLayoutSaveRestore::GetLayoutStringFromIni( UserLayoutString ));
		if ( UserLayout.IsValid() && UserLayout->GetPrimaryArea().IsValid() )
		{
			return UserLayout.ToSharedRef();
		}
	}

	return DefaultLayout;
}

FString FLayoutSaveRestore::PrepareLayoutStringForIni(const FString& LayoutString)
{
	// Have to store braces as parentheses due to braces causing ini issues
	return LayoutString.Replace(TEXT("{"), TEXT("(")).Replace(TEXT("}"), TEXT(")")).Replace(LINE_TERMINATOR, TEXT("\\")LINE_TERMINATOR);
}

FString FLayoutSaveRestore::GetLayoutStringFromIni(const FString& LayoutString)
{
	// Revert parenthesis to braces, from ini readable to Json readable
	return LayoutString.Replace(TEXT("("), TEXT("{")).Replace(TEXT(")"), TEXT("}")).Replace(TEXT("\\")LINE_TERMINATOR, LINE_TERMINATOR);
}
