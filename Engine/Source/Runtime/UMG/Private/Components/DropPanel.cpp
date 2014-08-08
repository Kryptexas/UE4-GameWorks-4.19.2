// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGDragDropOp.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UDropPanel

UDropPanel::UDropPanel(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDropPanel::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyDropPanel.Reset();
}

TSharedRef<SWidget> UDropPanel::RebuildWidget()
{
	MyDropPanel = SNew(SDropPanel)
		.OnDragEnter(BIND_UOBJECT_DELEGATE(FOnDragEnterEvent, HandleDragEnter))
		.OnDragLeave(BIND_UOBJECT_DELEGATE(FOnDragLeaveEvent, HandleDragLeave))
		.OnDragOver(BIND_UOBJECT_DELEGATE(FOnDragDropEvent, HandleDragOver))
		.OnDrop(BIND_UOBJECT_DELEGATE(FOnDragDropEvent, HandleDrop));

	if ( GetChildrenCount() > 0 )
	{
		MyDropPanel->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}
	
	return MyDropPanel.ToSharedRef();
}

void UDropPanel::SyncronizeProperties()
{
	Super::SyncronizeProperties();
}

void UDropPanel::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live slot if it already exists
	if ( MyDropPanel.IsValid() )
	{
		MyDropPanel->SetContent(Slot->Content ? Slot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UDropPanel::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyDropPanel.IsValid() )
	{
		MyDropPanel->SetContent(SNullWidget::NullWidget);
	}
}

void UDropPanel::HandleDragEnter(FGeometry MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if ( OnDragEnterEvent.IsBound() )
	{
		TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
		if ( NativeOp.IsValid() )
		{
			OnDragEnterEvent.Execute(MyGeometry, DragDropEvent, NativeOp->GetOperation());
		}
	}
}

void UDropPanel::HandleDragLeave(const FDragDropEvent& DragDropEvent)
{
	if ( OnDragLeaveEvent.IsBound() )
	{
		TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
		if ( NativeOp.IsValid() )
		{
			OnDragLeaveEvent.Execute(DragDropEvent, NativeOp->GetOperation());
		}
	}
}

FReply UDropPanel::HandleDragOver(FGeometry MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if ( OnDragOverEvent.IsBound() )
	{
		TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
		if ( NativeOp.IsValid() )
		{
			if ( OnDragOverEvent.Execute(MyGeometry, DragDropEvent, NativeOp->GetOperation()) )
			{
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

FReply UDropPanel::HandleDrop(FGeometry MyGeometry, const FDragDropEvent& DragDropEvent)
{
	if ( OnDropEvent.IsBound() )
	{
		TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
		if ( NativeOp.IsValid() )
		{
			if ( OnDropEvent.Execute(MyGeometry, DragDropEvent, NativeOp->GetOperation()) )
			{
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

#if WITH_EDITOR

const FSlateBrush* UDropPanel::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.DropPanel");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
