// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UScrollBoxSlot

UScrollBoxSlot::UScrollBoxSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Slot(NULL)
{
	HorizontalAlignment = HAlign_Left;
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
