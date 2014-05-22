// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UHorizontalBoxComponent

UHorizontalBoxComponent::UHorizontalBoxComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UHorizontalBoxComponent::GetChildrenCount() const
{
	return Slots.Num();
}

USlateWrapperComponent* UHorizontalBoxComponent::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

bool UHorizontalBoxComponent::AddChild(USlateWrapperComponent* Child, FVector2D Position)
{
	AddSlot(Child);
	return true;
}

bool UHorizontalBoxComponent::RemoveChild(USlateWrapperComponent* Child)
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UHorizontalBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Child )
		{
			Slots.RemoveAt(SlotIndex);
			return true;
		}
	}

	return false;
}

void UHorizontalBoxComponent::ReplaceChildAt(int32 Index, USlateWrapperComponent* Content)
{
	UHorizontalBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
}

TSharedRef<SWidget> UHorizontalBoxComponent::RebuildWidget()
{
	TSharedRef<SHorizontalBox> NewCanvas = SNew(SHorizontalBox);
	MyHorizontalBox = NewCanvas;

	TSharedPtr<SHorizontalBox> Canvas = MyHorizontalBox.Pin();
	if ( Canvas.IsValid() )
	{
		Canvas->ClearChildren();

		for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
		{
			UHorizontalBoxSlot* Slot = Slots[SlotIndex];
			if ( Slot == NULL )
			{
				Slots[SlotIndex] = Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
			}

			auto& NewSlot = Canvas->AddSlot()
				.Padding(Slot->Padding)
				.HAlign(Slot->HorizontalAlignment)
				.VAlign(Slot->VerticalAlignment)
				[
					Slot->Content == NULL ? SNullWidget::NullWidget : Slot->Content->GetWidget()
				];

			NewSlot.SizeParam = USlateWrapperComponent::ConvertSerializedSizeParamToRuntime(Slot->Size);
		}
	}

	return NewCanvas;
}

UHorizontalBoxSlot* UHorizontalBoxComponent::AddSlot(USlateWrapperComponent* Content)
{
	UHorizontalBoxSlot* Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
	Slot->Content = Content;

#if WITH_EDITOR
	Content->Slot = Slot;
#endif
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR
void UHorizontalBoxComponent::ConnectEditorData()
{
	for ( UHorizontalBoxSlot* Slot : Slots )
	{
		Slot->Content->Slot = Slot;
	}
}

void UHorizontalBoxComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// Ensure the slots have unique names
	int32 SlotNumbering = 1;

	TSet<FName> UniqueSlotNames;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if ( Slots[SlotIndex] == NULL )
		{
			Slots[SlotIndex] = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
		}
	}
}
#endif
