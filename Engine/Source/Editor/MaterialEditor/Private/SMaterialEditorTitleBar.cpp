// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MaterialEditorModule.h"
#include "MaterialEditor.h"
#include "SMaterialEditorTitleBar.h"

void SMaterialEditorTitleBar::Construct(const FArguments& InArgs)
{
	Visibility = EVisibility::HitTestInvisible;

	this->ChildSlot
	[
		SNew(SVerticalBox)
		// Title text/icon
		+SVerticalBox::Slot()
		.VAlign(VAlign_Top)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.FillWidth(1.f)
			.Padding(5)
			[
				SAssignNew(MaterialInfoList, SListView<TSharedPtr<FMaterialInfo>>)
				.ListItemsSource(InArgs._MaterialInfoList)
				.OnGenerateRow(this, &SMaterialEditorTitleBar::MakeMaterialInfoWidget)
				.SelectionMode( ESelectionMode::None )
			]
		]
	];
}

TSharedRef<ITableRow> SMaterialEditorTitleBar::MakeMaterialInfoWidget(TSharedPtr<FMaterialInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const int32 FontSize = 9;

	FMaterialInfo Info = *Item.Get();
	FLinearColor TextColor = Info.Color;
	FString Text = Info.Text;

	if( Text.IsEmpty() )
	{
		return
			SNew(STableRow< TSharedPtr<FMaterialInfo> >, OwnerTable)
			[
				SNew(SSpacer)
			];
	}
	else 
	{
		return
			SNew(STableRow< TSharedPtr<FMaterialInfo> >, OwnerTable)
			[
				SNew(STextBlock)
				.ColorAndOpacity(TextColor)
				.Font(FSlateFontInfo( FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize ))
				.Text( Text )
			];
	}
}

void SMaterialEditorTitleBar::RequestRefresh()
{
	MaterialInfoList->RequestListRefresh();
}
