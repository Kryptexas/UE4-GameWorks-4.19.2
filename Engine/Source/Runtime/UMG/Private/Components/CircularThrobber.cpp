// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UCircularThrobber

UCircularThrobber::UCircularThrobber(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SCircularThrobber::FArguments DefaultArgs;

	NumberOfPieces = DefaultArgs._NumPieces;
	Period = DefaultArgs._Period;
	Radius = DefaultArgs._Radius;
}

void UCircularThrobber::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyCircularThrobber.Reset();
}

TSharedRef<SWidget> UCircularThrobber::RebuildWidget()
{
	SCircularThrobber::FArguments DefaultArgs;

	MyCircularThrobber = SNew(SCircularThrobber)
		.PieceImage(GetPieceBrush())
		.NumPieces(NumberOfPieces)
		.Period(Period)
		.Radius(Radius);

	return MyCircularThrobber.ToSharedRef();
}

void UCircularThrobber::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	MyCircularThrobber->SetPieceImage(GetPieceBrush());
	MyCircularThrobber->SetNumPieces(NumberOfPieces);
	MyCircularThrobber->SetPeriod(Period);
	MyCircularThrobber->SetRadius(Radius);
}

const FSlateBrush* UCircularThrobber::GetPieceBrush() const
{
	if (PieceImage == NULL)
	{
		SCircularThrobber::FArguments DefaultArgs;
		return DefaultArgs._PieceImage;
	}
	return &PieceImage->Brush;
}

void UCircularThrobber::SetNumberOfPieces(int32 InNumberOfPieces)
{
	NumberOfPieces = InNumberOfPieces;
	if (MyCircularThrobber.IsValid())
	{
		MyCircularThrobber->SetNumPieces(InNumberOfPieces);
	}
}

void UCircularThrobber::SetPeriod(float InPeriod)
{
	Period = InPeriod;
	if (MyCircularThrobber.IsValid())
	{
		MyCircularThrobber->SetPeriod(InPeriod);
	}
}

void UCircularThrobber::SetRadius(float InRadius)
{
	Radius = InRadius;
	if (MyCircularThrobber.IsValid())
	{
		MyCircularThrobber->SetRadius(InRadius);
	}
}

void UCircularThrobber::SetPieceImage(USlateBrushAsset* InPieceImage)
{
	PieceImage = InPieceImage;
	if (MyCircularThrobber.IsValid())
	{
		MyCircularThrobber->SetPieceImage(GetPieceBrush());
	}
}


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
