// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UProgressBar

UProgressBar::UProgressBar(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BarFillType = EProgressBarFillType::LeftToRight;
	Percent = 0;
	FillColorAndOpacity = FLinearColor::White;
	BorderPadding = FVector2D(1, 0);
}

TSharedRef<SWidget> UProgressBar::RebuildWidget()
{
	MyProgressBar = SNew(SProgressBar)
		.BarFillType(BarFillType)
		.Percent(Percent)
		.FillColorAndOpacity(FillColorAndOpacity)
		.BorderPadding(BorderPadding);

	return MyProgressBar.ToSharedRef();
}

void UProgressBar::SetPercent(float InPercent)
{
	return MyProgressBar->SetPercent(InPercent);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
