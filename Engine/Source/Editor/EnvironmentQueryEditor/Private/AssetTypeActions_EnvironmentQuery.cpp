// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnvironmentQueryEditorPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"

#include "Editor/EnvironmentQueryEditor/Public/EnvironmentQueryEditorModule.h"
#include "Editor/EnvironmentQueryEditor/Public/IEnvironmentQueryEditor.h"

#include "AssetTypeActions_EnvironmentQuery.h"
#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_EnvironmentQuery::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Scripts = GetTypedWeakObjectPtrs<UEnvQuery>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("EnvironmentQuery_Edit", "Edit"),
		LOCTEXT("EnvironmentQuery_EditTooltip", "Opens the selected Environment Query in editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_EnvironmentQuery::ExecuteEdit, Scripts ),
		FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_EnvironmentQuery::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Script = Cast<UEnvQuery>(*ObjIt);
		if (Script != NULL)
		{
			FEnvironmentQueryEditorModule& EnvironmentQueryEditorModule = FModuleManager::LoadModuleChecked<FEnvironmentQueryEditorModule>( "EnvironmentQueryEditor" );
			TSharedRef< IEnvironmentQueryEditor > NewEditor = EnvironmentQueryEditorModule.CreateEnvironmentQueryEditor( EToolkitMode::Standalone, EditWithinLevelEditor, Script );
		}
	}
}

void FAssetTypeActions_EnvironmentQuery::ExecuteEdit(TArray<TWeakObjectPtr<UEnvQuery>> Objects)
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

UClass* FAssetTypeActions_EnvironmentQuery::GetSupportedClass() const
{ 
	return UEnvQuery::StaticClass(); 
}


#undef LOCTEXT_NAMESPACE
