// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// USizeBoxSlot

USizeBoxSlot::USizeBoxSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Padding = FMargin(0, 0);

	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;

	SBox::FArguments SizeBoxDefaults;
}

void USizeBoxSlot::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	SizeBox.Reset();
}

void USizeBoxSlot::BuildSlot(TSharedRef<SBox> InSizeBox)
{
	SizeBox = InSizeBox;

	SynchronizeProperties();

	SizeBox->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}

void USizeBoxSlot::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if ( SizeBox.IsValid() )
	{
		SizeBox->SetPadding(InPadding);
	}
}

void USizeBoxSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( SizeBox.IsValid() )
	{
		SizeBox->SetHAlign(InHorizontalAlignment);
	}
}

void USizeBoxSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( SizeBox.IsValid() )
	{
		SizeBox->SetVAlign(InVerticalAlignment);
	}
}

void USizeBoxSlot::SynchronizeProperties()
{
	SetPadding(Padding);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
}
