// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Toolkits/AssetEditorManager.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_VertexAnimation::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto VertexAnims = GetTypedWeakObjectPtrs<UVertexAnimation>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("VertexAnim_Edit", "Edit"),
		LOCTEXT("VertexAnim_EditTooltip", "Opens the selected vertex animation in Persona."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_VertexAnimation::ExecuteEdit, VertexAnims ),
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_VertexAnimation::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto VertexAnim = Cast<UVertexAnimation>(*ObjIt);
		if (VertexAnim != NULL && VertexAnim->BaseSkelMesh)
		{
		}
	}
}

void FAssetTypeActions_VertexAnimation::ExecuteEdit(TArray< TWeakObjectPtr<UVertexAnimation> > Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->BaseSkelMesh )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}

bool FAssetTypeActions_VertexAnimation::CanFilter()
{
	return false;
}

#undef LOCTEXT_NAMESPACE