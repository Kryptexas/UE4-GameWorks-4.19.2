// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UScrollBoxSlot

UScrollBoxSlot::UScrollBoxSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	HorizontalAlignment = HAlign_Fill;
}

void UScrollBoxSlot::BuildSlot(TSharedRef<SScrollBox> ScrollBox)
{
	Slot = &ScrollBox->AddSlot()
		.Padding(Padding)
		.HAlign(HorizontalAlignment)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->GetWidget()
		];
}

void UScrollBoxSlot::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if ( Slot )
	{
		Slot->Padding(InPadding);
	}
}

void UScrollBoxSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( Slot )
	{
		Slot->HAlign(InHorizontalAlignment);
	}
}

void UScrollBoxSlot::Refresh()
{
	SetPadding(Padding);
	SetHorizontalAlignment(HorizontalAlignment);
}
