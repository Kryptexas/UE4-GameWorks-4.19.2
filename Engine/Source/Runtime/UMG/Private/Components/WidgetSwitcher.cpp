// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UWidgetSwitcher

UWidgetSwitcher::UWidgetSwitcher(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = true;

	SWidgetSwitcher::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());
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

UClass* UWidgetSwitcher::GetSlotClass() const
{
	return UWidgetSwitcherSlot::StaticClass();
}

void UWidgetSwitcher::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyWidgetSwitcher.IsValid() )
	{
		Cast<UWidgetSwitcherSlot>(Slot)->BuildSlot(MyWidgetSwitcher.ToSharedRef());
	}
}

void UWidgetSwitcher::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyWidgetSwitcher.IsValid() )
	{
		MyWidgetSwitcher->RemoveSlot(Slot->Content->GetWidget());
	}
}

TSharedRef<SWidget> UWidgetSwitcher::RebuildWidget()
{
	MyWidgetSwitcher = SNew(SWidgetSwitcher);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UWidgetSwitcherSlot* TypedSlot = Cast<UWidgetSwitcherSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyWidgetSwitcher.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyWidgetSwitcher.ToSharedRef() );
}

void UWidgetSwitcher::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	SetActiveWidgetIndex(ActiveWidgetIndex);
}
