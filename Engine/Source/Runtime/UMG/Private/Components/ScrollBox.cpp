// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBox

UScrollBox::UScrollBox(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	Orientation = Orient_Vertical;

	SScrollBox::FArguments Defaults;
	Visiblity = UWidget::ConvertRuntimeToSerializedVisiblity(Defaults._Visibility.Get());

	WidgetStyle = *Defaults._Style;
	WidgetBarStyle = *Defaults._ScrollBarStyle;
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
	MyScrollBox = SNew(SScrollBox)
		.Style(&WidgetStyle)
		.ScrollBarStyle(&WidgetBarStyle)
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

void UScrollBox::SynchronizeProperties()
{
	MyScrollBox->SetScrollOffset(DesiredScrollOffset);
	MyScrollBox->SetOrientation(Orientation);
}

void UScrollBox::SetScrollOffset(float NewScrollOffset)
{
	DesiredScrollOffset = NewScrollOffset;

	if ( MyScrollBox.IsValid() )
	{
		MyScrollBox->SetScrollOffset(NewScrollOffset);
	}
}

void UScrollBox::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerUE4Version() < VER_UE4_DEPRECATE_UMG_STYLE_ASSETS )
	{
		if ( Style_DEPRECATED != nullptr )
		{
			const FScrollBoxStyle* StylePtr = Style_DEPRECATED->GetStyle<FScrollBoxStyle>();
			if ( StylePtr != nullptr )
			{
				WidgetStyle = *StylePtr;
			}

			Style_DEPRECATED = nullptr;
		}

		if ( BarStyle_DEPRECATED != nullptr )
		{
			const FScrollBarStyle* StylePtr = BarStyle_DEPRECATED->GetStyle<FScrollBarStyle>();
			if ( StylePtr != nullptr )
			{
				WidgetBarStyle = *StylePtr;
			}

			BarStyle_DEPRECATED = nullptr;
		}
	}
}

#if WITH_EDITOR

const FSlateBrush* UScrollBox::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ScrollBox");
}

const FText UScrollBox::GetPaletteCategory()
{
	return LOCTEXT("Panel", "Panel");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
