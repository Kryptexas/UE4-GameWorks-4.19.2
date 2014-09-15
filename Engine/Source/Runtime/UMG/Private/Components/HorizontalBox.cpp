// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UHorizontalBox

UHorizontalBox::UHorizontalBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SHorizontalBox::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());
}

void UHorizontalBox::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyHorizontalBox.Reset();
}

UClass* UHorizontalBox::GetSlotClass() const
{
	return UHorizontalBoxSlot::StaticClass();
}

void UHorizontalBox::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyHorizontalBox.IsValid() )
	{
		Cast<UHorizontalBoxSlot>(Slot)->BuildSlot(MyHorizontalBox.ToSharedRef());
	}
}

void UHorizontalBox::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyHorizontalBox.IsValid() )
	{
		TSharedPtr<SWidget> Widget = Slot->Content->GetCachedWidget();
		if ( Widget.IsValid() )
		{
			MyHorizontalBox->RemoveSlot(Widget.ToSharedRef());
		}
	}
}

UHorizontalBoxSlot* UHorizontalBox::AddSlot(UWidget* Content)
{
	return Cast<UHorizontalBoxSlot>( Super::AddChild(Content) );
}

UHorizontalBoxSlot* UHorizontalBox::HorizontalBoxSlot(UWidget* Child)
{
	return Cast<UHorizontalBoxSlot>(Child->Slot);
}

TSharedRef<SWidget> UHorizontalBox::RebuildWidget()
{
	MyHorizontalBox = SNew(SHorizontalBox);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UHorizontalBoxSlot* TypedSlot = Cast<UHorizontalBoxSlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyHorizontalBox.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyHorizontalBox.ToSharedRef() );
}

#if WITH_EDITOR

const FSlateBrush* UHorizontalBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.HorizontalBox");
}

const FText UHorizontalBox::GetPaletteCategory()
{
	return LOCTEXT("Panel", "Panel");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE