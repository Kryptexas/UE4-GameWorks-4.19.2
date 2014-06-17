// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UUniformGridPanel

UUniformGridPanel::UUniformGridPanel(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UUniformGridPanel::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UUniformGridPanel::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

bool UUniformGridPanel::AddChild(UWidget* Child, FVector2D Position)
{
	UUniformGridSlot* Slot = AddSlot(Child);
	
	// Add the child to the live panel if it already exists
	if (MyUniformGridPanel.IsValid())
	{
		Slot->BuildSlot(MyUniformGridPanel.ToSharedRef());
	}
	
	return true;
}

bool UUniformGridPanel::RemoveChild(UWidget* Child)
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UUniformGridSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Child )
		{
			Slots.RemoveAt(SlotIndex);
			
			// Remove the widget from the live slot if it exists.
			if (MyUniformGridPanel.IsValid())
			{
				MyUniformGridPanel->RemoveSlot(Child->GetWidget());
			}
			
			return true;
		}
	}

	return false;
}

void UUniformGridPanel::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UUniformGridSlot* Slot = Slots[Index];
	Slot->Content = Content;

	Content->Slot = Slot;
}

TSharedRef<SWidget> UUniformGridPanel::RebuildWidget()
{
	MyUniformGridPanel =
		SNew(SUniformGridPanel)
		.SlotPadding(SlotPadding)
		.MinDesiredSlotWidth(MinDesiredSlotWidth)
		.MinDesiredSlotHeight(MinDesiredSlotHeight);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot(MyUniformGridPanel.ToSharedRef());
	}

	return BuildDesignTimeWidget( MyUniformGridPanel.ToSharedRef() );
}

UUniformGridSlot* UUniformGridPanel::AddSlot(UWidget* Content)
{
	UUniformGridSlot* Slot = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR

void UUniformGridPanel::ConnectEditorData()
{
	for ( UUniformGridSlot* Slot : Slots )
	{
		Slot->Content->Slot = Slot;
		Slot->Parent = this;
	}
}

#endif
