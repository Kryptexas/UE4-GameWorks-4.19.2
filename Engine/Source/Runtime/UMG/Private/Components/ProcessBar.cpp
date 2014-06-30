// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UProgressBar

UProgressBar::UProgressBar(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BarFillType = EProgressBarFillType::LeftToRight;
	bIsMarquee = false;
	Percent = 0;
	FillColorAndOpacity = FLinearColor::White;
	BorderPadding = FVector2D(1, 0);
}

TSharedRef<SWidget> UProgressBar::RebuildWidget()
{
	MyProgressBar = SNew(SProgressBar);

	return MyProgressBar.ToSharedRef();
}

void UProgressBar::SyncronizeProperties()
{
	const FProgressBarStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FProgressBarStyle>() : NULL;
	if ( StylePtr )
	{
		MyProgressBar->SetStyle(StylePtr);
	}

	TAttribute< TOptional<float> > PercentBinding = OPTIONAL_BINDING_CONVERT(float, Percent, TOptional<float>, ConvertFloatToOptionalFloat);

	MyProgressBar->SetPercent(bIsMarquee ? TOptional<float>() : PercentBinding);
	
	MyProgressBar->SetBarFillType(BarFillType);
	MyProgressBar->SetFillColorAndOpacity(FillColorAndOpacity);
	MyProgressBar->SetBorderPadding(BorderPadding);
	
	MyProgressBar->SetBackgroundImage(BackgroundImage ? &BackgroundImage->Brush : nullptr);
	MyProgressBar->SetFillImage(FillImage ? &FillImage->Brush : nullptr);
	MyProgressBar->SetMarqueeImage(MarqueeImage ? &MarqueeImage->Brush : nullptr);
}

void UProgressBar::SetPercent(float InPercent)
{
	return MyProgressBar->SetPercent(InPercent);
}

#if WITH_EDITOR

const FSlateBrush* UProgressBar::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ProgressBar");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
