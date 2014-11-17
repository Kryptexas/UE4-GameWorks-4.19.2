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

void FAssetTypeActions_SlateWidgetStyle::OpenAssetEditor( const TArray<UObject*>& Objects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	struct Local
	{
		static TArray<UObject*> GetSubObjects(const TArray<UObject*>& InObjects)
		{
			TArray<UObject*> SubObjects;
			for(UObject* Object : InObjects)
			{
				auto Style = Cast<USlateWidgetStyleAsset>(Object);
				if(Style && Style->CustomStyle)
				{
					SubObjects.Add(Style->CustomStyle);
				}
			}
			return SubObjects;
		}
	};

	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Objects, FSimpleAssetEditor::FGetDetailsViewObjects::CreateStatic(&Local::GetSubObjects));
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