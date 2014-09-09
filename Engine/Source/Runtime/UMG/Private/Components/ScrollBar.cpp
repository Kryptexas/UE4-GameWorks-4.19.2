// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UScrollBar

UScrollBar::UScrollBar(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	bAlwaysShowScrollbar = true;
	Orientation = Orient_Vertical;
	Thickness = FVector2D(12.0f, 12.0f);
}

TSharedRef<SWidget> UScrollBar::RebuildWidget()
{
	const FScrollBarStyle* StylePtr = ( Style != nullptr ) ? Style->GetStyle<FScrollBarStyle>() : nullptr;
	if ( StylePtr == nullptr )
	{
		SScrollBar::FArguments Defaults;
		StylePtr = Defaults._Style;
	}

	MyScrollBar = SNew(SScrollBar)
		.Style(StylePtr)
		.AlwaysShowScrollbar(bAlwaysShowScrollbar)
		.Orientation(Orientation)
		.Thickness(Thickness);

	//SLATE_EVENT(FOnUserScrolled, OnUserScrolled)

	return MyScrollBar.ToSharedRef();
}

void UScrollBar::SynchronizeProperties()
{
	//MyScrollBar->SetScrollOffset(DesiredScrollOffset);
}

void UScrollBar::SetState(float InOffsetFraction, float InThumbSizeFraction)
{
	if ( MyScrollBar.IsValid() )
	{
		MyScrollBar->SetState(InOffsetFraction, InThumbSizeFraction);
	}
}

#if WITH_EDITOR

const FSlateBrush* UScrollBar::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ScrollBar");
}

const FText UScrollBar::GetToolboxCategory()
{
	return LOCTEXT("Primitive", "Primitive");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
