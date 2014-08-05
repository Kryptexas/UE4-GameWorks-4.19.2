// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "SDropPanel.h"
#include "UMGDragDropOp.h"


/* SDropPanel interface
 *****************************************************************************/

void SDropPanel::Construct( const SDropPanel::FArguments& InArgs )
{
	ChildSlot
	[
		InArgs._Content.Widget
	];
}

void SDropPanel::SetContent(TSharedRef<SWidget> InContent)
{
	ChildSlot
	[
		InContent
	];
}

void SDropPanel::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{

}

void SDropPanel::OnDragLeave(const FDragDropEvent& DragDropEvent)
{

}

FReply SDropPanel::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return FReply::Unhandled();
}

FReply SDropPanel::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	return FReply::Unhandled();
}
