// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "PackageTools.h"
#include "AssetRegistryModule.h"
#include "ISourceControlModule.h"
#include "BlueprintEditorModule.h"
#include "Editor/UnrealEd/Public/Kismet2/StructureEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Struct::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor )
{
	if (FStructureEditorUtils::UserDefinedStructEnabled())
	{
		const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );

		for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			if (UUserDefinedStruct* UDStruct = Cast<UUserDefinedStruct>(*ObjIt))
			{
				BlueprintEditorModule.CreateUserDefinedStructEditor(Mode, EditWithinLevelEditor, UDStruct);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
