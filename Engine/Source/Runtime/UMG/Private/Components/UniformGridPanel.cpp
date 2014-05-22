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

USlateWrapperComponent* UUniformGridPanel::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

bool UUniformGridPanel::AddChild(USlateWrapperComponent* Child, FVector2D Position)
{
	AddSlot(Child);
	return true;
}

bool UUniformGridPanel::RemoveChild(USlateWrapperComponent* Child)
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UUniformGridSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Child )
		{
			Slots.RemoveAt(SlotIndex);
			return true;
		}
	}

	return false;
}

void UUniformGridPanel::ReplaceChildAt(int32 Index, USlateWrapperComponent* Content)
{
	UUniformGridSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

TSharedRef<SWidget> UUniformGridPanel::RebuildWidget()
{
	TSharedRef<SUniformGridPanel> NewPanel = SNew(SUniformGridPanel);
	MyUniformGridPanel = NewPanel;

	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UUniformGridSlot* Slot = Slots[SlotIndex];
		if ( Slot == NULL )
		{
			Slots[SlotIndex] = Slot = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
		}

		auto& NewSlot = NewPanel->AddSlot(Slot->Column, Slot->Row)
			.HAlign(Slot->HorizontalAlignment)
			.VAlign(Slot->VerticalAlignment)
			[
				Slot->Content == NULL ? SNullWidget::NullWidget : Slot->Content->GetWidget()
			];
	}

	return NewPanel;
}

UUniformGridSlot* UUniformGridPanel::AddSlot(USlateWrapperComponent* Content)
{
	UUniformGridSlot* Slot = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR

void UUniformGridPanel::ConnectEditorData()
{
	for ( UUniformGridSlot* Slot : Slots )
	{
		Slot->Content->Slot = Slot;
	}
}

void UUniformGridPanel::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UUniformGridSlot>(UUniformGridSlot::StaticClass(), this);
		}
	}
}

#endif
