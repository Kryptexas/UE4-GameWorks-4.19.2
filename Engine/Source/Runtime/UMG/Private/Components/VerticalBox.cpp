// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UVerticalBox

UVerticalBox::UVerticalBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UVerticalBox::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UVerticalBox::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UVerticalBox::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UVerticalBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UVerticalBox::AddChild(UWidget* Child, FVector2D Position)
{
	UVerticalBoxSlot* Slot = AddSlot(Child);
	return true;
}

bool UVerticalBox::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		
		// Remove the widget from the live slot if it exists.
		if (MyVerticalBox.IsValid())
		{
			MyVerticalBox->RemoveSlot(Child->GetWidget());
		}
		
		return true;
	}

	return false;
}

void UVerticalBox::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UVerticalBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

	Content->Slot = Slot;
}

void UVerticalBox::InsertChildAt(int32 Index, UWidget* Content)
{
	UVerticalBoxSlot* Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UVerticalBox::RebuildWidget()
{
	MyVerticalBox = SNew(SVerticalBox);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot(MyVerticalBox.ToSharedRef());
	}

	return BuildDesignTimeWidget( MyVerticalBox.ToSharedRef() );
}

UVerticalBoxSlot* UVerticalBox::AddSlot(UWidget* Content)
{
	UVerticalBoxSlot* Slot = ConstructObject<UVerticalBoxSlot>(UVerticalBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;
	
	Slots.Add(Slot);

	// Add the child to the live canvas if it already exists
	if ( MyVerticalBox.IsValid() )
	{
		Slot->BuildSlot(MyVerticalBox.ToSharedRef());
	}

	return Slot;
}

#if WITH_EDITOR

void UVerticalBox::ConnectEditorData()
{
	for ( UVerticalBoxSlot* Slot : Slots )
	{
		// TODO UMG Find a better way to assign the parent pointers for slots.
		Slot->Parent = this;

		if ( Slot->Content )
		{
			Slot->Content->Slot = Slot;
		}
		//TODO UMG Should we auto delete empty slots?
	}
}

const FSlateBrush* UVerticalBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.VerticalBox");
}

#endif
