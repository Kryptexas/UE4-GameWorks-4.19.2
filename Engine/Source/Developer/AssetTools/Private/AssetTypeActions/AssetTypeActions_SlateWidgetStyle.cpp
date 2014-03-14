// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

#include "AssetTypeActions_SlateWidgetStyle.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


void FAssetTypeActions_SlateWidgetStyle::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Styles = GetTypedWeakObjectPtrs< USlateWidgetStyleAsset >(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SlateWidgetStyle_Edit", "Edit"),
		LOCTEXT("SlateWidgetStyle_EditTooltip", "Opens the selected styles in the style editor."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_SlateWidgetStyle::ExecuteEdit, Styles ),
		FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_SlateWidgetStyle::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	TArray<UObject*> SubObjects;
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Style = Cast< USlateWidgetStyleAsset >(*ObjIt);
		if ( Style != NULL && Style->CustomStyle != NULL )
		{
			SubObjects.Add( Style->CustomStyle );
		}
	}

	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, SubObjects);
}

void FAssetTypeActions_SlateWidgetStyle::ExecuteEdit(TArray<TWeakObjectPtr< USlateWidgetStyleAsset >> Styles)
{
	TArray<UObject*> Objects;
	for( auto StyleIter = Styles.CreateConstIterator(); StyleIter; ++StyleIter )
	{
		Objects.Add( StyleIter->Get() );
	}

	OpenAssetEditor( Objects );
}

#undef LOCTEXT_NAMESPACE