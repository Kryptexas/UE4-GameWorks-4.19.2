// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetSwitcher

UWidgetSwitcher::UWidgetSwitcher(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = true;
}

int32 UWidgetSwitcher::GetNumWidgets() const
{
	if ( MyWidgetSwitcher.IsValid() )
	{
		return MyWidgetSwitcher->GetNumWidgets();
	}

	return Slots.Num();
}

int32 UWidgetSwitcher::GetActiveWidgetIndex() const
{
	if ( MyWidgetSwitcher.IsValid() )
	{
		return MyWidgetSwitcher->GetActiveWidgetIndex();
	}

	return ActiveWidgetIndex;
}

void UWidgetSwitcher::SetActiveWidgetIndex(int32 Index)
{
	ActiveWidgetIndex = Index;
	if ( MyWidgetSwitcher.IsValid() )
	{
		// Ensure the index is clamped to a valid range.
		int32 SafeIndex = FMath::Clamp(Index, 0, FMath::Max(0, Slots.Num() - 1));
		MyWidgetSwitcher->SetActiveWidgetIndex(SafeIndex);
	}
}

int32 UWidgetSwitcher::GetChildrenCount() const
{
	return Slots.Num();
}

UWidget* UWidgetSwitcher::GetChildAt(int32 Index) const
{
	return Slots[Index]->Content;
}

int32 UWidgetSwitcher::GetChildIndex(UWidget* Content) const
{
	for ( int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex )
	{
		UWidgetSwitcherSlot* Slot = Slots[SlotIndex];

		if ( Slot->Content == Content )
		{
			return SlotIndex;
		}
	}

	return -1;
}

bool UWidgetSwitcher::AddChild(UWidget* Child, FVector2D Position)
{
	UWidgetSwitcherSlot* Slot = AddSlot(Child);
	return true;
}

bool UWidgetSwitcher::RemoveChild(UWidget* Child)
{
	int32 SlotIndex = GetChildIndex(Child);
	if ( SlotIndex != -1 )
	{
		Slots.RemoveAt(SlotIndex);
		
		// Remove the widget from the live slot if it exists.
		if (MyWidgetSwitcher.IsValid())
		{
			MyWidgetSwitcher->RemoveSlot(Child->GetWidget());
		}
		
		return true;
	}

	return false;
}

void UWidgetSwitcher::ReplaceChildAt(int32 Index, UWidget* Content)
{
	UWidgetSwitcherSlot* Slot = Slots[Index];
	Slot->Content = Content;

	Content->Slot = Slot;
}

void UWidgetSwitcher::InsertChildAt(int32 Index, UWidget* Content)
{
	UWidgetSwitcherSlot* Slot = ConstructObject<UWidgetSwitcherSlot>(UWidgetSwitcherSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;

	Slots.Insert(Slot, Index);
}

UWidgetSwitcherSlot* UWidgetSwitcher::AddSlot(UWidget* Content)
{
	UWidgetSwitcherSlot* Slot = ConstructObject<UWidgetSwitcherSlot>(UWidgetSwitcherSlot::StaticClass(), this);
	Slot->SetFlags(RF_Transactional);
	Slot->Content = Content;
	Slot->Parent = this;

	Content->Slot = Slot;

	Slots.Add(Slot);

	// Add the child to the live canvas if it already exists
	if ( MyWidgetSwitcher.IsValid() )
	{
		Slot->BuildSlot(MyWidgetSwitcher.ToSharedRef());
	}

	return Slot;
}

TSharedRef<SWidget> UWidgetSwitcher::RebuildWidget()
{
	MyWidgetSwitcher = SNew(SWidgetSwitcher);

	for ( auto Slot : Slots )
	{
		Slot->Parent = this;
		Slot->BuildSlot(MyWidgetSwitcher.ToSharedRef());
	}

	return BuildDesignTimeWidget( MyWidgetSwitcher.ToSharedRef() );
}

void UWidgetSwitcher::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	SetActiveWidgetIndex(ActiveWidgetIndex);
}

#if WITH_EDITOR

void UWidgetSwitcher::ConnectEditorData()
{
	for ( UWidgetSwitcherSlot* Slot : Slots )
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
