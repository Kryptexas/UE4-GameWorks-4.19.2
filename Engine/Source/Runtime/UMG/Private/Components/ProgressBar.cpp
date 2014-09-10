// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UProgressBar

UProgressBar::UProgressBar(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	static const FName StyleName(TEXT("Style"));
	ProgressStyle = PCIP.CreateDefaultSubobject<UProgressWidgetStyle>(this, StyleName);

	BarFillType = EProgressBarFillType::LeftToRight;
	bIsMarquee = false;
	Percent = 0;
	FillColorAndOpacity = FLinearColor::White;
}

void UProgressBar::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyProgressBar.Reset();
}

TSharedRef<SWidget> UProgressBar::RebuildWidget()
{
	MyProgressBar = SNew(SProgressBar)
		.BorderPadding(FVector2D(0.0f, 0.0f));

	return MyProgressBar.ToSharedRef();
}

void UProgressBar::SynchronizeProperties()
{
	TAttribute< TOptional<float> > PercentBinding = OPTIONAL_BINDING_CONVERT(float, Percent, TOptional<float>, ConvertFloatToOptionalFloat);

	MyProgressBar->SetStyle(&ProgressStyle->ProgressBarStyle);

	MyProgressBar->SetPercent(bIsMarquee ? TOptional<float>() : PercentBinding);
	
	MyProgressBar->SetBarFillType(BarFillType);
	MyProgressBar->SetFillColorAndOpacity(FillColorAndOpacity);
}

void UProgressBar::SetIsMarquee(bool InbIsMarquee)
{
	bIsMarquee = InbIsMarquee;
	if ( MyProgressBar.IsValid() )
	{
		MyProgressBar->SetPercent(bIsMarquee ? TOptional<float>() : Percent);
	}
}

void UProgressBar::SetPercent(float InPercent)
{
	Percent = InPercent;
	if ( MyProgressBar.IsValid() )
	{
		MyProgressBar->SetPercent(InPercent);
	}
}

void UProgressBar::PostLoad()
{
	Super::PostLoad();

	if ( GetLinkerUE4Version() < VER_UE4_DEPRECATE_UMG_STYLE_ASSETS )
	{
		if ( Style_DEPRECATED != nullptr )
		{
			const FProgressBarStyle* StylePtr = Style_DEPRECATED->GetStyle<FProgressBarStyle>();
			if ( StylePtr != nullptr )
			{
				ProgressStyle->ProgressBarStyle = *StylePtr;
			}

			Style_DEPRECATED = nullptr;
		}

		if ( BackgroundImage_DEPRECATED != nullptr )
		{
			ProgressStyle->ProgressBarStyle.BackgroundImage = BackgroundImage_DEPRECATED->Brush;
			BackgroundImage_DEPRECATED = nullptr;
		}

		if ( FillImage_DEPRECATED != nullptr )
		{
			ProgressStyle->ProgressBarStyle.FillImage = FillImage_DEPRECATED->Brush;
			FillImage_DEPRECATED = nullptr;
		}

		if ( MarqueeImage_DEPRECATED != nullptr )
		{
			ProgressStyle->ProgressBarStyle.MarqueeImage = MarqueeImage_DEPRECATED->Brush;
			MarqueeImage_DEPRECATED = nullptr;
		}
	}
}

#if WITH_EDITOR

const FSlateBrush* UProgressBar::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.ProgressBar");
}

const FText UProgressBar::GetToolboxCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
