// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "WidgetTemplateDragDropOp.h"

#define LOCTEXT_NAMESPACE "UMG"

TSharedRef<FWidgetTemplateDragDropOp> FWidgetTemplateDragDropOp::New(const TSharedPtr<FWidgetTemplateDescriptor>& InTemplate)
{
	TSharedRef<FWidgetTemplateDragDropOp> Operation = MakeShareable(new FWidgetTemplateDragDropOp());
	Operation->Template = InTemplate;
	Operation->DefaultHoverText = InTemplate->Name;
	Operation->CurrentHoverText = InTemplate->Name;
	Operation->Construct();

	return Operation;
}

//TSharedPtr<SWidget> FWidgetTemplateDragDropOp::GetDefaultDecorator() const
//{
//	return 
//		SNew(SBorder)
//		.BorderImage(FEditorStyle::GetBrush("ContentBrowser.AssetDragDropTooltipBackground"))
//		.Content()
//		[
//			SNew(SHorizontalBox)
//
//			+ SHorizontalBox::Slot()
//			.AutoWidth()
//			.HAlign(HAlign_Left)
//			[
//				SNew(STextBlock)
//				.Text(Template->Name)
//			]
//		];
//}

#undef LOCTEXT_NAMESPACE
