// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBox

UScrollBox::UScrollBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SScrollBox::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());
}

UClass* UScrollBox::GetSlotClass() const
{
	return UScrollBoxSlot::StaticClass();
}

void UScrollBox::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyScrollBox.IsValid() )
	{
		Cast<UScrollBoxSlot>(Slot)->BuildSlot(MyScrollBox.ToSharedRef());
	}
}

void UScrollBox::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyScrollBox.IsValid() )
	{
		MyScrollBox->RemoveSlot(Slot->Content->GetWidget());
	}
}

TSharedRef<SWidget> UScrollBox::RebuildWidget()
{
	const FScrollBoxStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FScrollBoxStyle>() : NULL;
	if ( StylePtr == NULL )
	{
		SScrollBox::FArguments Defaults;
		StylePtr = Defaults._Style;
	}

	const FScrollBarStyle* BarStylePtr = ( BarStyle != NULL ) ? BarStyle->GetStyle<FScrollBarStyle>() : NULL;
	if ( BarStylePtr == NULL )
	{
		SScrollBox::FArguments Defaults;
		BarStylePtr = Defaults._BarStyle;
	}

	MyScrollBox = SNew(SScrollBox)
		.Style(StylePtr)
		.BarStyle(BarStylePtr);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UScrollBoxSlot* TypedSlot = Cast<UScrollBoxSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyScrollBox.ToSharedRef());
		}
	}
	
	return BuildDesignTimeWidget( MyScrollBox.ToSharedRef() );
}

void UScrollBox::SyncronizeProperties()
{
	MyScrollBox->SetScrollOffset(DesiredScrollOffset);
}

void UScrollBox::ClearChildren()
{
	Slots.Reset();

	if ( MyScrollBox.IsValid() )
	{
		MyScrollBox->ClearChildren();
	}
}

void UScrollBox::SetScrollOffset(float NewScrollOffset)
{
	DesiredScrollOffset = NewScrollOffset;

	if ( MyScrollBox.IsValid() )
	{
		MyScrollBox->SetScrollOffset(NewScrollOffset);
	}
}

#if WITH_EDITOR

const FSlateBrush* UScrollBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ScrollBox");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
