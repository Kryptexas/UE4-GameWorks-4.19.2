// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UOverlay

UOverlay::UOverlay(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SOverlay::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());
}

void UOverlay::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyOverlay.Reset();
}

UClass* UOverlay::GetSlotClass() const
{
	return UOverlaySlot::StaticClass();
}

void UOverlay::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live canvas if it already exists
	if ( MyOverlay.IsValid() )
	{
		Cast<UOverlaySlot>(Slot)->BuildSlot(MyOverlay.ToSharedRef());
	}
}

void UOverlay::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyOverlay.IsValid() )
	{
		TSharedPtr<SWidget> Widget = Slot->Content->GetCachedWidget();
		if ( Widget.IsValid() )
		{
			MyOverlay->RemoveSlot(Widget.ToSharedRef());
		}
	}
}

TSharedRef<SWidget> UOverlay::RebuildWidget()
{
	MyOverlay = SNew(SOverlay);

	for ( UPanelSlot* Slot : Slots )
	{
		if ( UOverlaySlot* TypedSlot = Cast<UOverlaySlot>(Slot) )
		{
			TypedSlot->Parent = this;
			TypedSlot->BuildSlot(MyOverlay.ToSharedRef());
		}
	}

	return BuildDesignTimeWidget( MyOverlay.ToSharedRef() );
}

#if WITH_EDITOR

const FSlateBrush* UOverlay::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Overlay");
}

const FText UOverlay::GetToolboxCategory()
{
	return LOCTEXT("Panel", "Panel");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE