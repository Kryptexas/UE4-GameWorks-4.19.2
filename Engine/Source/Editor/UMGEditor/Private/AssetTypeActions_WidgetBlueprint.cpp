// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "PackageTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "BlueprintEditorModule.h"
#include "AssetRegistryModule.h"
#include "SBlueprintDiff.h"
#include "ISourceControlModule.h"
#include "MessageLog.h"

#include "AssetTypeActions_WidgetBlueprint.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_WidgetBlueprint::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Blueprint = Cast<UBlueprint>(*ObjIt);
		if (Blueprint && Blueprint->SkeletonGeneratedClass && Blueprint->GeneratedClass )
		{
			FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>( "Kismet" );
			TSharedRef< IBlueprintEditor > NewKismetEditor = BlueprintEditorModule.CreateBlueprintEditor( Mode, EditWithinLevelEditor, Blueprint, ShouldUseDataOnlyEditor(Blueprint) );
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToLoadBlueprint", "Blueprint could not be loaded because it derives from an invalid class.  Check to make sure the parent class for this blueprint hasn't been removed!"));
		}
	}
}

bool FAssetTypeActions_WidgetBlueprint::ShouldUseDataOnlyEditor(const UBlueprint* Blueprint) const
{
	return FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint)
		&& !FBlueprintEditorUtils::IsLevelScriptBlueprint(Blueprint)
		&& !FBlueprintEditorUtils::IsInterfaceBlueprint(Blueprint)
		&& !Blueprint->bForceFullEditor
		&& !Blueprint->bIsNewlyCreated;
}

#undef LOCTEXT_NAMESPACE