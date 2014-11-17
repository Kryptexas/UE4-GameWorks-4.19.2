// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UBorderSlot

UBorderSlot::UBorderSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Padding = FMargin(4, 2);

	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;

	SBorder::FArguments BorderDefaults;

	Padding = BorderDefaults._Padding.Get();
}

void UBorderSlot::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	Border.Reset();
}

void UBorderSlot::BuildSlot(TSharedRef<SBorder> InBorder)
{
	Border = InBorder;

	Border->SetPadding(Padding);
	Border->SetHAlign(HorizontalAlignment);
	Border->SetVAlign(VerticalAlignment);

	Border->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}

void UBorderSlot::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if ( Border.IsValid() )
	{
		Border->SetPadding(InPadding);
	}
}

void UBorderSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( Border.IsValid() )
	{
		Border->SetHAlign(InHorizontalAlignment);
	}
}

void UBorderSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( Border.IsValid() )
	{
		Border->SetVAlign(InVerticalAlignment);
	}
}

void UBorderSlot::SyncronizeProperties()
{
	SetPadding(Padding);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
}
