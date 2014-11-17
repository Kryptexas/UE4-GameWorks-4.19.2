// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// SConcreteBoxPanel

class SConcreteBoxPanel : public SBoxPanel
{
public:
	class FSlot : public SBoxPanel::FSlot
	{
	public:
		FSlot& SetSizeParam(const FSizeParam& InParam)
		{
			SizeParam = InParam;
			return *this;
		}

		FSlot& MaxWidth( const TAttribute< float >& InMaxWidth )
		{
			MaxSize = InMaxWidth;
			return *this;
		}

		FSlot& Padding( TAttribute<FMargin> InPadding )
		{
			SlotPadding = InPadding;
			return *this;
		}

		FSlot& HAlign( EHorizontalAlignment InHAlignment )
		{
			HAlignment = InHAlignment;
			return *this;
		}

		FSlot& VAlign( EVerticalAlignment InVAlignment )
		{
			VAlignment = InVAlignment;
			return *this;
		}

		FSlot& operator[]( TSharedRef<SWidget> InWidget )
		{
			Widget = InWidget;
			return *this;
		}

		FSlot& Expose( FSlot*& OutVarToInit )
		{
			OutVarToInit = this;
			return *this;
		}
	};

	static FSlot& Slot()
	{
		return *(new FSlot());
	}

	SLATE_BEGIN_ARGS( SConcreteBoxPanel )
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}
		SLATE_SUPPORTS_SLOT(SConcreteBoxPanel::FSlot)
	SLATE_END_ARGS()

	FSlot& AddSlot()
	{
		SConcreteBoxPanel::FSlot& NewSlot = SConcreteBoxPanel::Slot();
		this->Children.Add( &NewSlot );
		return NewSlot;
	}

	void Construct(const FArguments& InArgs, EOrientation InOrientation)
	{
		DirtyHackSetOrientation(InOrientation);

		const int32 NumSlots = InArgs.Slots.Num();
		for (int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex)
		{
			Children.Add(InArgs.Slots[SlotIndex]);
		}
	}

	SConcreteBoxPanel()
		: SBoxPanel(Orient_Horizontal)
	{
	}

protected:
	//@TODO: Make Orientation not const in the base class; it's already protected and AFAIK there are no
	// negative consequences from altering it before the first tick/computedesiredsize call
	void DirtyHackSetOrientation(EOrientation InOrientation)
	{
		EOrientation& NonConstOrientation = *(const_cast<EOrientation*>(&Orientation));
		NonConstOrientation = InOrientation;
	}
};

/////////////////////////////////////////////////////
// UBoxPanelComponent

UBoxPanelComponent::UBoxPanelComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

TSharedRef<SWidget> UBoxPanelComponent::RebuildWidget()
{
	SConcreteBoxPanel::FArguments Args;

	TSharedRef<SConcreteBoxPanel> Box = SNew(SConcreteBoxPanel, Orientation);
	MyBox = Box;

	return Box;
}

#if WITH_EDITOR
void UBoxPanelComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		FName& SlotName = Slots[SlotIndex].SlotName;

		if ((SlotName == NAME_None) || UniqueSlotNames.Contains(SlotName))
		{
			do 
			{
				SlotName = FName(TEXT("Slot"), SlotNumbering++);
			} while (UniqueSlotNames.Contains(SlotName));
		}

		UniqueSlotNames.Add(SlotName);
	}
}
#endif