// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UHorizontalBox

UHorizontalBox::UHorizontalBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UHorizontalBox::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UHorizontalBox::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UHorizontalBox::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UHorizontalBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UHorizontalBox::AddChild(UWidget* Child, FVector2D Position)
{
	UHorizontalBoxSlot* Slot = AddSlot(Child);
	return true;
}

bool UHorizontalBox::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		
		// Remove the widget from the live slot if it exists.
		if (MyHorizontalBox.IsValid())
		{
			MyHorizontalBox->RemoveSlot(Child->GetWidget());
		}
		
		return true;
	}

	return false;
}

void UHorizontalBox::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UHorizontalBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

	Content->Slot = Slot;
}

void UHorizontalBox::InsertChildAt(int32 Index, UWidget* Content)
{
	UHorizontalBoxSlot* Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UHorizontalBox::RebuildWidget()
{
	MyHorizontalBox = SNew(SHorizontalBox);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot( MyHorizontalBox.ToSharedRef() );
	}

	return BuildDesignTimeWidget( MyHorizontalBox.ToSharedRef() );
}

UHorizontalBoxSlot* UHorizontalBox::AddSlot(UWidget* Content)
{
	UHorizontalBoxSlot* Slot = ConstructObject<UHorizontalBoxSlot>(UHorizontalBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;
	
	Slots.Add(Slot);

	// Add the child to the live canvas if it already exists
	if ( MyHorizontalBox.IsValid() )
	{
		Slot->BuildSlot(MyHorizontalBox.ToSharedRef());
	}

	return Slot;
}

#if WITH_EDITOR

void UHorizontalBox::ConnectEditorData()
{
	for ( UHorizontalBoxSlot* Slot : Slots )
	{
		Slot->Content->Slot = Slot;
		Slot->Parent = this;
	}
}

const FSlateBrush* UHorizontalBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.HorizontalBox");
}

#endif
