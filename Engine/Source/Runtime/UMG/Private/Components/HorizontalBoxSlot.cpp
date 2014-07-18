// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UHorizontalBoxSlot

UHorizontalBoxSlot::UHorizontalBoxSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	HorizontalAlignment = HAlign_Fill;
	VerticalAlignment = VAlign_Fill;
	Size = FSlateChildSize(ESlateSizeRule::Automatic);
}

void UHorizontalBoxSlot::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	Slot = NULL;
}

void UHorizontalBoxSlot::BuildSlot(TSharedRef<SHorizontalBox> HorizontalBox)
{
	Slot = &HorizontalBox->AddSlot()
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		.Padding(Padding)
		[
			Content == NULL ? SNullWidget::NullWidget : Content->GetWidget()
		];

	Slot->SizeParam = UWidget::ConvertSerializedSizeParamToRuntime(Size);
}

void UHorizontalBoxSlot::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	if ( Slot )
	{
		Slot->Padding(InPadding);
	}
}

void UHorizontalBoxSlot::SetSize(FSlateChildSize InSize)
{
	Size = InSize;
	if ( Slot )
	{
		Slot->SizeParam = UWidget::ConvertSerializedSizeParamToRuntime(InSize);
	}
}

void UHorizontalBoxSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	if ( Slot )
	{
		Slot->HAlign(InHorizontalAlignment);
	}
}

void UHorizontalBoxSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	if ( Slot )
	{
		Slot->VAlign(InVerticalAlignment);
	}
}

void UHorizontalBoxSlot::SyncronizeProperties()
{
	SetPadding(Padding);
	SetSize(Size);
	SetHorizontalAlignment(HorizontalAlignment);
	SetVerticalAlignment(VerticalAlignment);
}
