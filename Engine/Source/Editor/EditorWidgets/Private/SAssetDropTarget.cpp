// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EditorWidgetsPrivatePCH.h"
#include "SAssetDropTarget.h"
#include "AssetDragDropOp.h"
#include "ActorDragDropOp.h"
#include "AssetSelection.h"
#include "SScaleBox.h"

#define LOCTEXT_NAMESPACE "EditorWidgets"

void SAssetDropTarget::Construct(const FArguments& InArgs )
{
	OnAssetDropped = InArgs._OnAssetDropped;
	OnIsAssetAcceptableForDrop = InArgs._OnIsAssetAcceptableForDrop;

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
			.Visibility(this, &SAssetDropTarget::GetDragOverlayVisibility)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(this, &SAssetDropTarget::GetDropBorderColor)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(this, &SAssetDropTarget::GetBackgroundBrightness)
				.Padding(15.0f)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					[
						SNew(STextBlock)
						.Text(this, &SAssetDropTarget::GetDragOverlayText)
						.ColorAndOpacity(this, &SAssetDropTarget::GetDropBorderColor)
					]
				]
			]
		]
	];
}

FSlateColor SAssetDropTarget::GetBackgroundBrightness() const
{
	return ( bAllowDrop && bIsDragOver ) ? FLinearColor(1, 1, 1, 0.40f) : FLinearColor(1, 1, 1, 0.25f);
}

EVisibility SAssetDropTarget::GetDragOverlayVisibility() const
{
	if ( FSlateApplication::Get().IsDragDropping() )
	{
		if ( AllowDrop(FSlateApplication::Get().GetDragDroppingContent()) || bIsDragOver )
		{
			return EVisibility::HitTestInvisible;
		}
	}

	return EVisibility::Hidden;
}

FText SAssetDropTarget::GetDragOverlayText() const
{
	return bAllowDrop ? FText::GetEmpty() : LOCTEXT("Nope", "Nope");
}

FSlateColor SAssetDropTarget::GetDropBorderColor() const
{
	return bIsDragEventRecognized ? 
		bAllowDrop ? FLinearColor(0,1,0,1) : FLinearColor(1,0,0,1)
		: FLinearColor::White;
}

FReply SAssetDropTarget::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// Handle the reply if we are allowed to drop, otherwise do not handle it.
	return AllowDrop(DragDropEvent.GetOperation()) ? FReply::Handled() : FReply::Unhandled();
}

bool SAssetDropTarget::AllowDrop(TSharedPtr<FDragDropOperation> DragDropOperation) const
{
	bool bRecognizedEvent = false;
	UObject* Object = GetDroppedObject(DragDropOperation, bRecognizedEvent);

	// Not being dragged over by a recognizable event
	bIsDragEventRecognized = bRecognizedEvent;

	bAllowDrop = false;

	if ( Object )
	{
		// Check and see if its valid to drop this object
		if ( OnIsAssetAcceptableForDrop.IsBound() )
		{
			bAllowDrop = OnIsAssetAcceptableForDrop.Execute(Object);
		}
		else
		{
			// If no delegate is bound assume its always valid to drop this object
			bAllowDrop = true;
		}
	}
	else
	{
		// No object so we dont allow dropping
		bAllowDrop = false;
	}

	return bAllowDrop;
}

FReply SAssetDropTarget::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// We've dropped an asset so we are no longer being dragged over
	bIsDragEventRecognized = false;

	// if we allow drop, call a delegate to handle the drop
	if( bAllowDrop )
	{
		bool bUnused;
		UObject* Object = GetDroppedObject( DragDropEvent.GetOperation(), bUnused );

		if( Object )
		{
			OnAssetDropped.ExecuteIfBound( Object );
		}
	}

	return FReply::Handled();
}

void SAssetDropTarget::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	// initially we dont recognize this event
	bIsDragEventRecognized = false;
	bIsDragOver = true;
}

void SAssetDropTarget::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
	// No longer being dragged over
	bIsDragEventRecognized = false;
	// Disallow dropping if not dragged over.
	bAllowDrop = false;

	bIsDragOver = false;
}

UObject* SAssetDropTarget::GetDroppedObject(TSharedPtr<FDragDropOperation> DragDropOperation, bool& bOutRecognizedEvent) const
{
	bOutRecognizedEvent = false;
	UObject* DroppedObject = NULL;

	// Asset being dragged from content browser
	if ( DragDropOperation->IsOfType<FAssetDragDropOp>() )
	{
		bOutRecognizedEvent = true;
		TSharedPtr<FAssetDragDropOp> DragDropOp = StaticCastSharedPtr<FAssetDragDropOp>(DragDropOperation);

		bool bCanDrop = DragDropOp->AssetData.Num() == 1;

		if( bCanDrop )
		{
			const FAssetData& AssetData = DragDropOp->AssetData[0];

			// Make sure the asset is loaded
			DroppedObject = AssetData.GetAsset();
		}
	}
	// Asset being dragged from some external source
	else if ( DragDropOperation->IsOfType<FExternalDragOperation>() )
	{
		TArray<FAssetData> DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag(DragDropOperation);

		if (DroppedAssetData.Num() == 1)
		{
			bOutRecognizedEvent = true;
			DroppedObject = DroppedAssetData[0].GetAsset();
		}
	}
	// Actor being dragged?
	else if ( DragDropOperation->IsOfType<FActorDragDropOp>() )
	{
		bOutRecognizedEvent = true;
		TSharedPtr<FActorDragDropOp> ActorDragDrop = StaticCastSharedPtr<FActorDragDropOp>(DragDropOperation);

		if (ActorDragDrop->Actors.Num() == 1)
		{
			DroppedObject = ActorDragDrop->Actors[0].Get();
		}
	}

	return DroppedObject;
}

int32 SAssetDropTarget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
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

#undef LOCTEXT_NAMESPACE
