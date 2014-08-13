// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UButtonSlot

UButtonSlot::UButtonSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Padding = FMargin(4, 2);

	HorizontalAlignment = HAlign_Center;
	VerticalAlignment = VAlign_Center;
}

void UButtonSlot::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	Button.Reset();
}

void UButtonSlot::BuildSlot(TSharedRef<SButton> InButton)
{
	Button = InButton;

	Button->SetContentPadding(Padding);
	Button->SetHAlign(HorizontalAlignment);
	Button->SetVAlign(VerticalAlignment);

	Button->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}

void UButtonSlot::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if ( Button.IsValid() )
	{
		Button->SetContentPadding(InPadding);
	}
}

void UButtonSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( Button.IsValid() )
	{
		Button->SetHAlign(InHorizontalAlignment);
	}
}

void UButtonSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( Button.IsValid() )
	{
		Button->SetVAlign(InVerticalAlignment);
	}
}

void UButtonSlot::SynchronizeProperties()
{
	SetPadding(Padding);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
}
