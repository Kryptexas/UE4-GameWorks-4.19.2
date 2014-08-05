// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "SDragPanel.h"
#include "UMGDragDropOp.h"


/* SDragPanel interface
 *****************************************************************************/

void SDragPanel::Construct( const SDragPanel::FArguments& InArgs )
{
	OnDragOperationRequested = InArgs._OnDragDetected;

	ChildSlot
	[
		InArgs._Content.Widget
	];
}

void SDragPanel::SetContent(TSharedRef<SWidget> InContent)
{
	ChildSlot
	[
		InContent
	];
}

FReply SDragPanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	// TODO UMG The button should be configurable
	if ( PointerEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}
	
	return FReply::Unhandled();
}

FReply SDragPanel::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	if ( OnDragOperationRequested.IsBound() )
	{
		TSharedPtr<FDragDropOperation> DragOperation = OnDragOperationRequested.Execute(MyGeometry, PointerEvent);

		if ( DragOperation.IsValid() )
		{
			return FReply::Handled().BeginDragDrop(DragOperation.ToSharedRef());
		}
	}

	return FReply::Unhandled();
}