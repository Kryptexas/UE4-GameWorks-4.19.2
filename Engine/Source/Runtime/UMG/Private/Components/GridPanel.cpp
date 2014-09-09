// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UGridPanel

UGridPanel::UGridPanel(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SGridPanel::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());
}

void UGridPanel::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyGridPanel.Reset();
}

UClass* UGridPanel::GetSlotClass() const
{
	return UGridSlot::StaticClass();
}

void UGridPanel::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyGridPanel.IsValid() )
	{
		Cast<UGridSlot>(Slot)->BuildSlot(MyGridPanel.ToSharedRef());
	}
}

void UGridPanel::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyGridPanel.IsValid() )
	{
		TSharedPtr<SWidget> Widget = Slot->Content->GetCachedWidget();
		if ( Widget.IsValid() )
		{
			MyGridPanel->RemoveSlot(Widget.ToSharedRef());
		}
	}
}

TSharedRef<SWidget> UGridPanel::RebuildWidget()
{
	MyGridPanel = SNew(SGridPanel);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UGridSlot* TypedSlot = Cast<UGridSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot( MyGridPanel.ToSharedRef() );
		}
	}

	return BuildDesignTimeWidget( MyGridPanel.ToSharedRef() );
}

void UGridPanel::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	for ( int ColumnIndex = 0; ColumnIndex < ColumnFill.Num(); ColumnIndex++ )
	{
		MyGridPanel->SetColumnFill(ColumnIndex, ColumnFill[ColumnIndex]);
	}

	for ( int RowIndex = 0; RowIndex < RowFill.Num(); RowIndex++ )
	{
		MyGridPanel->SetRowFill(RowIndex, RowFill[RowIndex]);
	}
}

#if WITH_EDITOR

const FSlateBrush* UGridPanel::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Grid");
}

const FText UGridPanel::GetToolboxCategory()
{
	return LOCTEXT("Panel", "Panel");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
