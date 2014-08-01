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
	Orientation = Defaults._Orientation;
}

void UScrollBox::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyScrollBox.Reset();
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
		TSharedPtr<SWidget> Widget = Slot->Content->GetCachedWidget();
		if ( Widget.IsValid() )
		{
			MyScrollBox->RemoveSlot(Widget.ToSharedRef());
		}
	}
}

TSharedRef<SWidget> UScrollBox::RebuildWidget()
{
	const FScrollBoxStyle* StylePtr = ( Style != nullptr ) ? Style->GetStyle<FScrollBoxStyle>() : nullptr;
	if ( StylePtr == nullptr )
	{
		SScrollBox::FArguments Defaults;
		StylePtr = Defaults._Style;
	}

	const FScrollBarStyle* BarStylePtr = ( BarStyle != nullptr ) ? BarStyle->GetStyle<FScrollBarStyle>() : nullptr;
	if ( BarStylePtr == nullptr )
	{
		SScrollBox::FArguments Defaults;
		BarStylePtr = Defaults._ScrollBarStyle;
	}

	MyScrollBox = SNew(SScrollBox)
		.Style(StylePtr)
		.ScrollBarStyle(BarStylePtr)
		.Orientation(Orientation);

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
	MyScrollBox->SetOrientation(Orientation);
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
