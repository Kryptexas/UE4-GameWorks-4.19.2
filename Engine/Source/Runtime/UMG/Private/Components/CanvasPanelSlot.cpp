// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UCanvasPanelSlot

UCanvasPanelSlot::UCanvasPanelSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	LayoutData.Offsets = FMargin(0, 0, 100, 30);
	LayoutData.Anchors = FAnchors(0.0f, 0.0f);
	LayoutData.Alignment = FVector2D(0.0f, 0.0f);
}

void UCanvasPanelSlot::BuildSlot(TSharedRef<SConstraintCanvas> Canvas)
{
	Slot = &Canvas->AddSlot()
		.Offset(LayoutData.Offsets)
		.Anchors(LayoutData.Anchors)
		.Alignment(LayoutData.Alignment)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->GetWidget()
		];
}

void UCanvasPanelSlot::SetDesiredPosition(FVector2D InPosition)
{
	SetOffset(FMargin(InPosition.X, InPosition.Y, LayoutData.Offsets.Right, LayoutData.Offsets.Bottom));
}

void UCanvasPanelSlot::SetDesiredSize(FVector2D InSize)
{
	SetOffset(FMargin(LayoutData.Offsets.Left, LayoutData.Offsets.Top, InSize.X, InSize.Y));
}

void UCanvasPanelSlot::Resize(const FVector2D& Direction, const FVector2D& Amount)
{
	if ( Direction.X < 0 )
	{
		LayoutData.Offsets.Left -= Amount.X * Direction.X;

		// Keep the right side stable if it's not stretched and we're resizing left.
		if ( !LayoutData.Anchors.IsStretchedHorizontal() )
		{
			// TODO UMG This doesn't work correctly with Alignment, fix or remove alignment
			LayoutData.Offsets.Right += Amount.X * Direction.X;
		}
	}
	
	if ( Direction.Y < 0 )
	{
		LayoutData.Offsets.Top -= Amount.Y * Direction.Y;

		// Keep the bottom side stable if it's not stretched and we're resizing top.
		if ( !LayoutData.Anchors.IsStretchedVertical() )
		{
			// TODO UMG This doesn't work correctly with Alignment, fix or remove alignment
			LayoutData.Offsets.Bottom += Amount.Y * Direction.Y;
		}
	}
	
	if ( Direction.X > 0 )
	{
		if ( LayoutData.Anchors.IsStretchedHorizontal() )
		{
			LayoutData.Offsets.Right -= Amount.X * Direction.X;
		}
		else
		{
			LayoutData.Offsets.Right += Amount.X * Direction.X;
		}
	}

	if ( Direction.Y > 0 )
	{
		if ( LayoutData.Anchors.IsStretchedVertical() )
		{
			LayoutData.Offsets.Bottom -= Amount.Y * Direction.Y;
		}
		else
		{
			LayoutData.Offsets.Bottom += Amount.Y * Direction.Y;
		}
	}

	if ( Slot )
	{
		Slot->Offset(LayoutData.Offsets);
	}
}

bool UCanvasPanelSlot::CanResize(const FVector2D& Direction) const
{
	return true;
}

void UCanvasPanelSlot::SetOffset(FMargin InOffset)
{
	LayoutData.Offsets = InOffset;
	if ( Slot )
	{
		Slot->Offset(InOffset);
	}
}

void UCanvasPanelSlot::SetAnchors(FAnchors InAnchors)
{
	LayoutData.Anchors = InAnchors;
	if ( Slot )
	{
		Slot->Anchors(InAnchors);
	}
}

void UCanvasPanelSlot::SetAlignment(FVector2D InAlignment)
{
	LayoutData.Alignment = InAlignment;
	if ( Slot )
	{
		Slot->Alignment(InAlignment);
	}
}

void UCanvasPanelSlot::Refresh()
{
	SetOffset(LayoutData.Offsets);
	SetAnchors(LayoutData.Anchors);
	SetAlignment(LayoutData.Alignment);
}

#if WITH_EDITOR

void UCanvasPanelSlot::PreEditChange(UProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);

	// Get the current location
	if ( UCanvasPanel* Canvas = Cast<UCanvasPanel>(Parent) )
	{
		FGeometry Geometry;
		if ( Canvas->GetGeometryForSlot(this, Geometry) )
		{
			PreEditGeometry = Geometry;
			PreEditLayoutData = LayoutData;
		}
	}
}

void UCanvasPanelSlot::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Refresh();

	static FName AnchorsProperty(TEXT("Anchors"));
	
	FEditPropertyChain::TDoubleLinkedListNode* AnchorNode = PropertyChangedEvent.PropertyChain.GetHead()->GetNextNode();
	if (!AnchorNode)
	{
		return;
	}
	
	FEditPropertyChain::TDoubleLinkedListNode* LayoutDataNode = AnchorNode->GetNextNode();
	
	if (!LayoutDataNode)
	{
		return;
	}
	
	UProperty* AnchorProperty = LayoutDataNode->GetValue();

	if ( AnchorProperty && AnchorProperty->GetFName() == AnchorsProperty )
	{
		// Ensure we have a parent canvas
		if ( UCanvasPanel* Canvas = Cast<UCanvasPanel>(Parent) )
		{
			FGeometry Geometry;
			if ( Canvas->GetGeometryForSlot(this, Geometry) )
			{
				// Calculate the default anchor offset, ie where would this control be laid out if no offset were provided.
				FVector2D CanvasSize = Canvas->GetCanvasWidget()->GetCachedGeometry().Size;
				FMargin AnchorPositions = FMargin(
					LayoutData.Anchors.Minimum.X * CanvasSize.X,
					LayoutData.Anchors.Minimum.Y * CanvasSize.Y,
					LayoutData.Anchors.Maximum.X * CanvasSize.X,
					LayoutData.Anchors.Maximum.Y * CanvasSize.Y);
				FVector2D DefaultAnchorPosition = FVector2D(AnchorPositions.Left, AnchorPositions.Top);

				// Determine the amount that would be offset from the anchor position if alignment was applied.
				FVector2D AlignmentOffset = LayoutData.Alignment * PreEditGeometry.Size;

				// Determine where the widget's new position needs to be to maintain a stable location when the anchors change.
				FVector2D PositionDelta = PreEditGeometry.Position - DefaultAnchorPosition;// ( Geometry.Position - DefaultAnchorOffset );

				// Adjust the size to remain constant
				if ( !LayoutData.Anchors.IsStretchedHorizontal() && PreEditLayoutData.Anchors.IsStretchedHorizontal() )
				{
					// Adjust the position to remain constant
					LayoutData.Offsets.Left = PositionDelta.X + AlignmentOffset.X;
					LayoutData.Offsets.Right = PreEditGeometry.Size.X;
				}
				else if ( LayoutData.Anchors.IsStretchedHorizontal() )
				{
					// Adjust the position to remain constant
					LayoutData.Offsets.Left = PositionDelta.X;
					LayoutData.Offsets.Right = AnchorPositions.Right - ( PositionDelta.X + PreEditGeometry.Size.X );
				}
				else
				{
					// Adjust the position to remain constant
					LayoutData.Offsets.Left = PositionDelta.X + AlignmentOffset.X;
				}

				if ( !LayoutData.Anchors.IsStretchedVertical() && PreEditLayoutData.Anchors.IsStretchedVertical() )
				{
					// Adjust the position to remain constant
					LayoutData.Offsets.Top = PositionDelta.Y + AlignmentOffset.Y;
					LayoutData.Offsets.Bottom = PreEditGeometry.Size.Y;
				}
				else if ( LayoutData.Anchors.IsStretchedVertical() )
				{
					// Adjust the position to remain constant
					LayoutData.Offsets.Top = PositionDelta.Y;
					LayoutData.Offsets.Bottom = AnchorPositions.Bottom - ( PositionDelta.Y + PreEditGeometry.Size.Y );
				}
				else
				{
					// Adjust the position to remain constant
					LayoutData.Offsets.Top = PositionDelta.Y + AlignmentOffset.Y;
				}
			}

			// Apply the changes to the properties.
			Refresh();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif