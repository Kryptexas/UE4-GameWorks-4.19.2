// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "SDropPanel.h"
#include "UMGDragDropOp.h"


/* SDropPanel interface
 *****************************************************************************/

void SDropPanel::Construct( const SDropPanel::FArguments& InArgs )
{
	OnDragEnterDelegate = InArgs._OnDragEnter;
	OnDragLeaveDelegate = InArgs._OnDragLeave;
	OnDragOverDelegate = InArgs._OnDragOver;
	OnDropDelegate = InArgs._OnDrop;

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
	if ( OnDragEnterDelegate.IsBound() )
	{
		OnDragEnterDelegate.Execute(MyGeometry, DragDropEvent);
	}
}

void SDropPanel::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	if ( OnDragLeaveDelegate.IsBound() )
	{
		OnDragLeaveDelegate.Execute(DragDropEvent);
	}
}

FReply SDropPanel::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if ( OnDragOverDelegate.IsBound() )
	{
		return OnDragOverDelegate.Execute(MyGeometry, DragDropEvent);
	}

	return FReply::Unhandled();
}

FReply SDropPanel::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if ( OnDropDelegate.IsBound() )
	{
		return OnDropDelegate.Execute(MyGeometry, DragDropEvent);
	}

	return FReply::Unhandled();
}
