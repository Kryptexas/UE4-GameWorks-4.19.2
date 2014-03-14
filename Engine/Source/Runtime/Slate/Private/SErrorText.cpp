// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

void SErrorText::Construct(const FArguments& InArgs)
{
	ExpandAnimation = FCurveSequence(0.0f, 0.15f);

	CustomVisibility = Visibility;
	Visibility = TAttribute<EVisibility>( SharedThis(this), &SErrorText::MyVisibility );

	SBorder::Construct( SBorder::FArguments()
		.BorderBackgroundColor( InArgs._BackgroundColor )
		.BorderImage( FCoreStyle::Get().GetBrush("ErrorReporting.Box") )
		.ContentScale( this, &SErrorText::GetDesiredSizeScale )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding( FMargin(3,0) )
		[
			SAssignNew( TextBlock, STextBlock )
			.ColorAndOpacity( FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor") )
		]
	);

	SetError( InArgs._ErrorText );
}

void SErrorText::SetError( const FText& InErrorText )
{
	SetError( InErrorText.ToString() );
}

void SErrorText::SetError( const FString& InErrorText )
{
	if ( TextBlock->GetText().IsEmpty() && !InErrorText.IsEmpty() )
	{
		ExpandAnimation.Play();
	}

	TextBlock->SetToolTipText( InErrorText );
	TextBlock->SetText( InErrorText );
}

bool SErrorText::HasError() const
{
	return ! TextBlock->GetText().IsEmpty();
}

TSharedRef<SWidget> SErrorText::AsWidget()
{
	return SharedThis(this);
}

EVisibility SErrorText::MyVisibility() const
{
	return (!TextBlock->GetText().IsEmpty())
		? CustomVisibility.Get()
		: EVisibility::Collapsed;
}

FVector2D SErrorText::GetDesiredSizeScale() const
{
	const float AnimAmount = ExpandAnimation.GetLerp();
	return FVector2D( 1.0f, AnimAmount );
}