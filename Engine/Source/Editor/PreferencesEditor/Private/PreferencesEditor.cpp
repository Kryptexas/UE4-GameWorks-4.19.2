// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "Slate.h"
#include "PreferencesEditor.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "PropertyEditing.h"
#include "PreferencesDetails.h"

IMPLEMENT_MODULE( FPreferencesEditor, PreferencesEditor );

void FPreferencesEditor::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyModule.RegisterCustomPropertyLayout( 
		UEditorUserSettings::StaticClass(), 
		FOnGetDetailCustomizationInstance::CreateStatic(&FPreferencesDetails::MakeInstance)
	);
}

void FPreferencesEditor::ShutdownModule()
{
	if( FModuleManager::Get().IsModuleLoaded("PropertyEditor") && FModuleManager::Get().IsModuleLoaded("UnrealEd") )
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.UnregisterCustomPropertyLayout( UEditorUserSettings::StaticClass() );
	}
}
