// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGDragDropOp.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UDragPanel

UDragPanel::UDragPanel(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDragPanel::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyDragPanel.Reset();
}

TSharedRef<SWidget> UDragPanel::RebuildWidget()
{
	MyDragPanel = SNew(SDragPanel)
		.OnDragDetected(BIND_UOBJECT_DELEGATE(FOnDragOperationRequested, HandleDragDetected));

	if ( GetChildrenCount() > 0 )
	{
		MyDragPanel->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}
	
	return MyDragPanel.ToSharedRef();
}

void UDragPanel::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live slot if it already exists
	if ( MyDragPanel.IsValid() )
	{
		MyDragPanel->SetContent(Slot->Content ? Slot->Content->TakeWidget() : SNullWidget::NullWidget);
	}
}

void UDragPanel::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyDragPanel.IsValid() )
	{
		MyDragPanel->SetContent(SNullWidget::NullWidget);
	}
}

TSharedPtr<FDragDropOperation> UDragPanel::HandleDragDetected(FGeometry Geometry, FPointerEvent PointerEvent)
{
	if ( OnDragEvent.IsBound() )
	{
		UDragDropOperation* Operation = NULL;

		OnDragEvent.Execute(Geometry, PointerEvent, Operation);

		FVector2D ScreenCursorPos = PointerEvent.GetScreenSpacePosition();
		FVector2D ScreenDrageePosition = Geometry.AbsolutePosition;

		return FUMGDragDropOp::New(Operation, ScreenCursorPos, ScreenDrageePosition);
	}

	return TSharedPtr<FDragDropOperation>();
}

#if WITH_EDITOR

const FSlateBrush* UDragPanel::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.DragPanel");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
