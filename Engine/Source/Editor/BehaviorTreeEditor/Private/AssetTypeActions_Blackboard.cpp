// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"

#include "BehaviorTree/BlackboardData.h"
#include "Editor/BehaviorTreeEditor/Public/BehaviorTreeEditorModule.h"
#include "Editor/BehaviorTreeEditor/Public/IBehaviorTreeEditor.h"

#include "AssetTypeActions_Blackboard.h"
#include "AssetTypeActions_BehaviorTree.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_Blackboard::GetSupportedClass() const
{
	return UBlackboardData::StaticClass(); 
}

bool FAssetTypeActions_Blackboard::HasActions ( const TArray<UObject*>& InObjects ) const
{
	return FAssetTypeActions_BehaviorTree::BehaviorTreeEditorEnabled();
}

void FAssetTypeActions_Blackboard::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	if (!FAssetTypeActions_BehaviorTree::BehaviorTreeEditorEnabled())
	{
		return;
	}

	auto BlackboardData = GetTypedWeakObjectPtrs<UBlackboardData>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Blackboard_Edit", "Edit"),
		LOCTEXT("Blackboard_EditTooltip", "Opens the selected Blackboard in editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_Blackboard::ExecuteEdit, BlackboardData ),
		FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_Blackboard::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor )
{
	if (!FAssetTypeActions_BehaviorTree::BehaviorTreeEditorEnabled())
	{
		return;
	}

	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for(auto Object : InObjects)
	{
		auto BlackboardData = Cast<UBlackboardData>(Object);
		if(BlackboardData != nullptr)
		{
			FBehaviorTreeEditorModule& BehaviorTreeEditorModule = FModuleManager::LoadModuleChecked<FBehaviorTreeEditorModule>( "BehaviorTreeEditor" );
			BehaviorTreeEditorModule.CreateBehaviorTreeEditor( EToolkitMode::Standalone, EditWithinLevelEditor, BlackboardData );
		}
	}	
}

void FAssetTypeActions_Blackboard::ExecuteEdit(TArray<TWeakObjectPtr<class UBlackboardData>> Objects)
{
	for(auto Object : Objects)
	{
		auto BlackboardData = Object.Get();
		if(BlackboardData != nullptr)
		{
			FAssetEditorManager::Get().OpenEditorForAsset(BlackboardData);
		}
	}
}

#undef LOCTEXT_NAMESPACE
