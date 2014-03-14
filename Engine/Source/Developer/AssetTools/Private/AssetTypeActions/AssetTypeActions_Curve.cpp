// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"
#include "CurveAssetEditorModule.h"
#include "ICurveAssetEditor.h"
#include "Editor/UnrealEd/Public/MiniCurveEditor.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_Curve::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Curves = GetTypedWeakObjectPtrs<UCurveBase>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Curve_Edit", "Edit"),
		LOCTEXT("Curve_EditTooltip", "Opens the selected curves in the curve editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_Curve::ExecuteEdit, Curves ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("Curve_Reimport", "Reimport"),
		LOCTEXT("Curve_ReimportTooltip", "Reimports the selected Curve from file."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_Curve::ExecuteReimport, Curves ),
		FCanExecuteAction::CreateSP(this, &FAssetTypeActions_Curve::CanReimportCurves, Curves) //if it was from a file?
		)
		);
}

void FAssetTypeActions_Curve::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Curve = Cast<UCurveBase>(*ObjIt);
		if (Curve != NULL)
		{
			FCurveAssetEditorModule& CurveAssetEditorModule = FModuleManager::LoadModuleChecked<FCurveAssetEditorModule>( "CurveAssetEditor" );
			TSharedRef< ICurveAssetEditor > NewCurveAssetEditor = CurveAssetEditorModule.CreateCurveAssetEditor( Mode, EditWithinLevelEditor, Curve );
		}
	}
}

void FAssetTypeActions_Curve::ExecuteEdit(TArray<TWeakObjectPtr<UCurveBase>> Objects)
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

void FAssetTypeActions_Curve::ExecuteReimport( TArray<TWeakObjectPtr<UCurveBase>> Objects )
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FReimportManager::Instance()->Reimport(Object, /*bAskForNewFileIfMissing=*/true);
		}
	}
}

bool FAssetTypeActions_Curve::CanReimportCurves( TArray<TWeakObjectPtr<UCurveBase>> Objects )const
{
	for(auto It = Objects.CreateIterator();It;++It)
	{
		auto& Obj = *It;
		if(UCurveBase* Curve = Obj.Get())
		{
			if(!Curve->ImportPath.IsEmpty())
			{
				return true;
			}
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
