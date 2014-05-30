// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UCanvasPanelSlot

UCanvasPanelSlot::UCanvasPanelSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	Position = FVector2D::ZeroVector;
	Size = FVector2D(1.0f, 1.0f);
	HorizontalAlignment = HAlign_Left;
	VerticalAlignment = VAlign_Top;
}

void UCanvasPanelSlot::BuildSlot(TSharedRef<SCanvas> Canvas)
{
	Slot = &Canvas->AddSlot()
		.Position(Position)
		.Size(Size)
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->GetWidget()
		];
}

void UCanvasPanelSlot::Resize(const FVector2D& Direction, const FVector2D& Amount)
{
	if ( Direction.X < 0 )
	{
		Position.X -= Amount.X * Direction.X;
		Size.X += Amount.X * Direction.X;
	}
	if ( Direction.Y < 0 )
	{
		Position.Y -= Amount.Y * Direction.Y;
		Size.Y += Amount.Y * Direction.Y;
	}
	if ( Direction.X > 0 )
	{
		Size.X += Amount.X * Direction.X;
	}
	if ( Direction.Y > 0 )
	{
		Size.Y += Amount.Y * Direction.Y;
	}

	if ( Slot )
	{
		Slot->Position(Position);
		Slot->Size(Size);
	}
}

bool UCanvasPanelSlot::CanResize(const FVector2D& Direction) const
{
	return true;
}

void UCanvasPanelSlot::SetPosition(FVector2D InPosition)
{
	Position = InPosition;
	if ( Slot )
	{
		Slot->Position(InPosition);
	}
}
