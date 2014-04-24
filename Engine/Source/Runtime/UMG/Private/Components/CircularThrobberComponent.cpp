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
	PieceImage = *DefaultArgs._PieceImage;
}

TSharedRef<SWidget> UCircularThrobberComponent::RebuildWidget()
{
	return SNew(SCircularThrobber)
		.PieceImage(&PieceImage)
		.NumPieces(NumberOfPieces)
		.Period(Period)
		.Radius(Radius);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
