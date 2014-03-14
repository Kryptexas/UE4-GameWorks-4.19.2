// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "SkeletonNotifyDetails.h"

#define LOCTEXT_NAMESPACE "SkeletonNotifyDetails"

TSharedRef<IDetailCustomization> FSkeletonNotifyDetails::MakeInstance()
{
	return MakeShareable( new FSkeletonNotifyDetails );
}

void FSkeletonNotifyDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Skeleton Notify", TEXT("Skeleton Notify") );
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();

	Category.AddProperty("Name").DisplayName( TEXT("Notify Name") );

	TSharedPtr<IPropertyHandle> InPropertyHandle = DetailBuilder.GetProperty("AnimationNames");

	TArray< TWeakObjectPtr<UObject> > SelectedObjects =  DetailBuilder.GetDetailsView().GetSelectedObjects();

	UEditorSkeletonNotifyObj* EdObj = NULL;
	for(int i = 0; i < SelectedObjects.Num(); ++i)
	{
		UObject* Obj = SelectedObjects[0].Get();
		EdObj = Cast<UEditorSkeletonNotifyObj>(Obj);
		if(EdObj)
		{
			break;
		}
	}

	if(EdObj)
	{
		Category.AddCustomRow(TEXT("Animations"))
		.NameContent()
		[
			SNew(STextBlock)
			.ToolTipText(LOCTEXT("Animations_Tooltip", "List of animations that reference this notify").ToString())
			.Text( LOCTEXT("AnimationsLabel","Animations").ToString() )
			.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SNew(SListView<TSharedPtr<FString>>)
			.ListItemsSource(&EdObj->AnimationNames)
			.OnGenerateRow(this, &FSkeletonNotifyDetails::MakeAnimationRow)
		];
	}
}

TSharedRef< ITableRow > FSkeletonNotifyDetails::MakeAnimationRow( TSharedPtr<FString> Item, const TSharedRef< STableViewBase >& OwnerTable )
{
	return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
	[
		SNew(STextBlock).Text(*Item.Get())
	];
}

#undef LOCTEXT_NAMESPACE
