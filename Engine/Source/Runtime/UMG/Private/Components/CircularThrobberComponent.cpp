// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UCircularThrobberComponent

UCircularThrobberComponent::UCircularThrobberComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SCircularThrobber::FArguments DefaultArgs;

	NumberOfPieces = DefaultArgs._NumPieces;
	Period = DefaultArgs._Period;
	Radius = DefaultArgs._Radius;
}

TSharedRef<SWidget> UCircularThrobberComponent::RebuildWidget()
{
	SCircularThrobber::FArguments DefaultArgs;

	const FSlateBrush* Image = PieceImage ? &PieceImage->Brush : DefaultArgs._PieceImage;
	
	return SNew(SCircularThrobber)
		.PieceImage(Image)
		.NumPieces(NumberOfPieces)
		.Period(Period)
		.Radius(Radius);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
