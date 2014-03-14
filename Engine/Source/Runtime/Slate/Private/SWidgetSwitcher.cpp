// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SWidgetSwitcher.cpp: Implements the SWidgetSwitcher class.
=============================================================================*/

 #include "Slate.h"
 #include "LayoutUtils.h"


/* SWidgetSwitcher interface
 *****************************************************************************/

FSimpleSlot& SWidgetSwitcher::AddSlot( int32 SlotIndex )
{
	FSimpleSlot& NewSlot = SWidgetSwitcher::Slot();

	if (!AllChildren.IsValidIndex(SlotIndex))
	{
		AllChildren.Add(&NewSlot);
	}
	else
	{
		AllChildren.Insert(&NewSlot, SlotIndex);
	}

	return NewSlot;
}


void SWidgetSwitcher::Construct( const FArguments& InArgs )
{
	OneDynamicChild = FOneDynamicChild( &AllChildren, &WidgetIndex );

	for (int32 Index = 0; Index < InArgs.Slots.Num(); ++Index)
	{
		AllChildren.Add(InArgs.Slots[Index]);
	}

	WidgetIndex = InArgs._WidgetIndex;
}


TSharedPtr<SWidget> SWidgetSwitcher::GetActiveWidget( ) const
{
	const int32 ActiveWidgetIndex = WidgetIndex.Get();
	if (ActiveWidgetIndex >= 0)
	{
		return AllChildren[ActiveWidgetIndex].Widget;
	}

	return NULL;
}


TSharedPtr<SWidget> SWidgetSwitcher::GetWidget( int32 SlotIndex ) const
{
	if (AllChildren.IsValidIndex(SlotIndex))
	{
		return AllChildren[SlotIndex].Widget;
	}

	return NULL;
}


int32 SWidgetSwitcher::GetWidgetIndex( TSharedRef<SWidget> Widget ) const
{
	for (int32 Index = 0; Index < AllChildren.Num(); ++Index)
	{
		const FSimpleSlot& Slot = AllChildren[Index];

		if (Slot.Widget == Widget)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void SWidgetSwitcher::SetActiveWidgetIndex( int32 Index )
{
	WidgetIndex = Index;
}

TAttribute<FVector2D> ContentScale = FVector2D::UnitVector;


/* SCompoundWidget interface
 *****************************************************************************/

void SWidgetSwitcher::ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	if (AllChildren.Num() > 0)
	{
		ArrangeSingleChild(AllottedGeometry, ArrangedChildren, AllChildren[WidgetIndex.Get()], ContentScale);
	}
}
	
FVector2D SWidgetSwitcher::ComputeDesiredSize( ) const
{
	return AllChildren.Num() > 0
		? AllChildren[WidgetIndex.Get()].Widget->GetDesiredSize()
		: FVector2D::ZeroVector;
}

FChildren* SWidgetSwitcher::GetChildren( )
{
	return &OneDynamicChild;
}
