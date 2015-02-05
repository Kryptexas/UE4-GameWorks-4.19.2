// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EditorWidgetsPrivatePCH.h"

#include "SDropTarget.h"
#include "AssetDragDropOp.h"
#include "ActorDragDropOp.h"
#include "AssetSelection.h"
#include "SScaleBox.h"

#define LOCTEXT_NAMESPACE "EditorWidgets"

void SDropTarget::Construct(const FArguments& InArgs)
{
	DroppedEvent = InArgs._OnDrop;
	AllowDropEvent = InArgs._OnAllowDrop;
	IsRecognizedEvent = InArgs._OnIsRecognized;

	bIsDragEventRecognized = false;
	bAllowDrop = false;
	bIsDragOver = false;

	ChildSlot
	[
		SNew(SOverlay)
			
		+ SOverlay::Slot()
		[
			InArgs._Content.Widget
		]

		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.Visibility(this, &SDropTarget::GetDragOverlayVisibility)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(this, &SDropTarget::GetDropBorderColor)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &SDropTarget::GetBackgroundBrightness)
				.Padding(15.0f)
			]
		]
	];
}

FSlateColor SDropTarget::GetBackgroundBrightness() const
{
	return ( bIsDragOver ) ? FLinearColor(1, 1, 1, 0.50f) : FLinearColor(1, 1, 1, 0.25f);
}

EVisibility SDropTarget::GetDragOverlayVisibility() const
{
	if ( FSlateApplication::Get().IsDragDropping() )
	{
		if ( AllowDrop(FSlateApplication::Get().GetDragDroppingContent()) || (bIsDragOver && bIsDragEventRecognized) )
		{
			return EVisibility::HitTestInvisible;
		}
	}

	return EVisibility::Hidden;
}

FSlateColor SDropTarget::GetDropBorderColor() const
{
	return bIsDragEventRecognized ? 
		bAllowDrop ? FLinearColor(0,1,0,1) : FLinearColor(1,0,0,1)
		: FLinearColor::White;
}

FReply SDropTarget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Handle the reply if we are allowed to drop, otherwise do not handle it.
	return AllowDrop(DragDropEvent.GetOperation()) ? FReply::Handled() : FReply::Unhandled();
}

bool SDropTarget::AllowDrop(TSharedPtr<FDragDropOperation> DragDropOperation) const
{
	bAllowDrop = OnAllowDrop(DragDropOperation);
	bIsDragEventRecognized = OnIsRecognized(DragDropOperation) || bAllowDrop;

	return bAllowDrop;
}

bool SDropTarget::OnAllowDrop(TSharedPtr<FDragDropOperation> DragDropOperation) const
{
	if ( AllowDropEvent.IsBound() )
	{
		return AllowDropEvent.Execute(DragDropOperation);
	}

	return false;
}

bool SDropTarget::OnIsRecognized(TSharedPtr<FDragDropOperation> DragDropOperation) const
{
	if ( IsRecognizedEvent.IsBound() )
	{
		return IsRecognizedEvent.Execute(DragDropOperation);
	}

	return false;
}

FReply SDropTarget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	const bool bCurrentbAllowDrop = bAllowDrop;

	// We've dropped an asset so we are no longer being dragged over
	bIsDragEventRecognized = false;
	bIsDragOver = false;
	bAllowDrop = false;

	// if we allow drop, call a delegate to handle the drop
	if ( bCurrentbAllowDrop )
	{
		if ( DroppedEvent.IsBound() )
		{
			return DroppedEvent.Execute(DragDropEvent.GetOperation());
		}
	}

	return FReply::Handled();
}

void SDropTarget::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// initially we dont recognize this event
	bIsDragEventRecognized = false;
	bIsDragOver = true;
}

void SDropTarget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	// No longer being dragged over
	bIsDragEventRecognized = false;
	// Disallow dropping if not dragged over.
	bAllowDrop = false;

	bIsDragOver = false;
}

int32 SDropTarget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if ( GetDragOverlayVisibility().IsVisible() )
	{
		if ( bIsDragEventRecognized )
		{
			FLinearColor DashColor = bAllowDrop ? FLinearColor(0, 1, 0, 1) : FLinearColor(1, 0, 0, 1);

			const FSlateBrush* HorizontalBrush = FEditorStyle::GetBrush("WideDash.Horizontal");
			const FSlateBrush* VerticalBrush = FEditorStyle::GetBrush("WideDash.Vertical");

			int32 DashLayer = LayerId + 1;

			// Top
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				DashLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(0, 0), FVector2D(AllottedGeometry.Size.X, HorizontalBrush->ImageSize.Y)),
				HorizontalBrush,
				MyClippingRect,
				ESlateDrawEffect::None,
				DashColor);

			// Bottom
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				DashLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(0, AllottedGeometry.Size.Y - HorizontalBrush->ImageSize.Y), FVector2D(AllottedGeometry.Size.X, HorizontalBrush->ImageSize.Y)),
				HorizontalBrush,
				MyClippingRect,
				ESlateDrawEffect::None,
				DashColor);

			// Left
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				DashLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(0, 0), FVector2D(VerticalBrush->ImageSize.X, AllottedGeometry.Size.Y)),
				VerticalBrush,
				MyClippingRect,
				ESlateDrawEffect::None,
				DashColor);

			// Right
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				DashLayer,
				AllottedGeometry.ToPaintGeometry(FVector2D(AllottedGeometry.Size.X - VerticalBrush->ImageSize.X, 0), FVector2D(VerticalBrush->ImageSize.X, AllottedGeometry.Size.Y)),
				VerticalBrush,
				MyClippingRect,
				ESlateDrawEffect::None,
				DashColor);

			return DashLayer;
		}
	}

	return LayerId;
}
