// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"

#include "Editor/BehaviorTreeEditor/Public/BehaviorTreeEditorModule.h"
#include "Editor/BehaviorTreeEditor/Public/IBehaviorTreeEditor.h"

#include "AssetTypeActions_BehaviorTree.h"
#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_BehaviorTree::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	bool BehaviorTreeNewAssetsEnabled=false;
	GConfig->GetBool(TEXT("BehaviorTreesEd"), TEXT("BehaviorTreeNewAssetsEnabled"), BehaviorTreeNewAssetsEnabled, GEngineIni);

	if (!BehaviorTreeNewAssetsEnabled && !GetDefault<UEditorExperimentalSettings>()->bBehaviorTreeEditor)
	{
		return;
	}

	auto Scripts = GetTypedWeakObjectPtrs<UBehaviorTree>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("BehaviorTree_Edit", "Edit"),
		LOCTEXT("BehaviorTree_EditTooltip", "Opens the selected Behavior Tree in editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_BehaviorTree::ExecuteEdit, Scripts ),
		FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_BehaviorTree::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	bool BehaviorTreeNewAssetsEnabled=false;
	GConfig->GetBool(TEXT("BehaviorTreesEd"), TEXT("BehaviorTreeNewAssetsEnabled"), BehaviorTreeNewAssetsEnabled, GEngineIni);

	if (!BehaviorTreeNewAssetsEnabled && !GetDefault<UEditorExperimentalSettings>()->bBehaviorTreeEditor)
	{
		return;
	}

	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Script = Cast<UBehaviorTree>(*ObjIt);
		if (Script != NULL)
		{
			FBehaviorTreeEditorModule& BehaviorTreeEditorModule = FModuleManager::LoadModuleChecked<FBehaviorTreeEditorModule>( "BehaviorTreeEditor" );
			TSharedRef< IBehaviorTreeEditor > NewEditor = BehaviorTreeEditorModule.CreateBehaviorTreeEditor( EToolkitMode::Standalone, EditWithinLevelEditor, Script );
		}
	}
}

void FAssetTypeActions_BehaviorTree::ExecuteEdit(TArray<TWeakObjectPtr<UBehaviorTree>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

UClass* FAssetTypeActions_BehaviorTree::GetSupportedClass() const
{ 
	return UBehaviorTree::StaticClass(); 
}


#undef LOCTEXT_NAMESPACE
