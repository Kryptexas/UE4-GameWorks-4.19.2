// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UCanvasPanelSlot

UCanvasPanelSlot::UCanvasPanelSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	Offset = FMargin(0, 0, 1, 1);
	Anchors = FAnchors(0.5f, 0.5f);
	Alignment = FVector2D(0.5f, 0.5f);
}

void UCanvasPanelSlot::BuildSlot(TSharedRef<SConstraintCanvas> Canvas)
{
	Slot = &Canvas->AddSlot()
		.Offset(Offset)
		.Anchors(Anchors)
		.Alignment(Alignment)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->GetWidget()
		];
}

void UCanvasPanelSlot::Resize(const FVector2D& Direction, const FVector2D& Amount)
{
	if ( Direction.X < 0 )
	{
		Offset.Left -= Amount.X * Direction.X;
		Offset.Right += Amount.X * Direction.X;
	}
	if ( Direction.Y < 0 )
	{
		Offset.Top -= Amount.Y * Direction.Y;
		Offset.Bottom += Amount.Y * Direction.Y;
	}
	if ( Direction.X > 0 )
	{
		Offset.Right += Amount.X * Direction.X;
	}
	if ( Direction.Y > 0 )
	{
		Offset.Bottom += Amount.Y * Direction.Y;
	}

	if ( Slot )
	{
		Slot->Offset(Offset);
	}
}

bool UCanvasPanelSlot::CanResize(const FVector2D& Direction) const
{
	return true;
}

void UCanvasPanelSlot::SetOffset(FMargin InOffset)
{
	Offset = InOffset;
	if ( Slot )
	{
		Slot->Offset(InOffset);
	}
}

void UCanvasPanelSlot::SetAnchors(FAnchors InAnchors)
{
	Anchors = InAnchors;
	if ( Slot )
	{
		Slot->Anchors(InAnchors);
	}
}

void UCanvasPanelSlot::SetAlignment(FVector2D InAlignment)
{
	Alignment = InAlignment;
	if ( Slot )
	{
		Slot->Alignment(InAlignment);
	}
}

#if WITH_EDITOR
void UCanvasPanelSlot::PreEditChange(UProperty* PropertyAboutToChange)
{
	//static FName AnchorsProperty(TEXT("Anchors"));

	//if ( PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == AnchorsProperty )
	//{

	//}
}

void UCanvasPanelSlot::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
}
#endif