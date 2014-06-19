// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBox

UScrollBox::UScrollBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;
}

int32 UScrollBox::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UScrollBox::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UScrollBox::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UScrollBoxSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UScrollBox::AddChild(UWidget* Child, FVector2D Position)
{
	UScrollBoxSlot* Slot = AddSlot(Child);
	
	// Add the child to the live canvas if it already exists
	if (MyScrollBox.IsValid())
	{
		Slot->BuildSlot(MyScrollBox.ToSharedRef());
	}
	
	return true;
}

bool UScrollBox::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		
		// Remove the widget from the live slot if it exists.
		if (MyScrollBox.IsValid())
		{
			MyScrollBox->RemoveSlot(Child->GetWidget());
		}
		
		return true;
	}

	return false;
}

void UScrollBox::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UScrollBoxSlot* Slot = Slots[Index];
	Slot->Content = Content;

	Content->Slot = Slot;
}

void UScrollBox::InsertChildAt(int32 Index, UWidget* Content)
{
	UScrollBoxSlot* Slot = ConstructObject<UScrollBoxSlot>(UScrollBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;

	Slots.Insert(Slot, Index);
}

TSharedRef<SWidget> UScrollBox::RebuildWidget()
{
	MyScrollBox = SNew(SScrollBox);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot(MyScrollBox.ToSharedRef());
	}
	
	return BuildDesignTimeWidget( MyScrollBox.ToSharedRef() );
}

UScrollBoxSlot* UScrollBox::AddSlot(UWidget* Content)
{
	UScrollBoxSlot* Slot = ConstructObject<UScrollBoxSlot>(UScrollBoxSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;
	
	Slots.Add(Slot);

	return Slot;
}

#if WITH_EDITOR

const FSlateBrush* UScrollBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ScrollBox");
}

void UScrollBox::ConnectEditorData()
{
	for ( UScrollBoxSlot* Slot : Slots )
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

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
