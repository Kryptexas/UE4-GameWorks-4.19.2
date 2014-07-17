// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UThrobber

UThrobber::UThrobber(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SThrobber::FArguments DefaultArgs;

	NumberOfPieces = DefaultArgs._NumPieces;

	bAnimateVertically = (DefaultArgs._Animate & SThrobber::Vertical) != 0;
	bAnimateHorizontally = (DefaultArgs._Animate & SThrobber::Horizontal) != 0;
	bAnimateOpacity = (DefaultArgs._Animate & SThrobber::Opacity) != 0;
}

TSharedRef<SWidget> UThrobber::RebuildWidget()
{
	SThrobber::FArguments DefaultArgs;

	MyThrobber = SNew(SThrobber)
		.PieceImage(GetPieceBrush())
		.NumPieces(NumberOfPieces)
		.Animate(GetAnimation());

	return MyThrobber.ToSharedRef();
}

void UThrobber::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	MyThrobber->SetPieceImage(GetPieceBrush());
	MyThrobber->SetNumPieces(NumberOfPieces);
	MyThrobber->SetAnimate(GetAnimation());
}

SThrobber::EAnimation UThrobber::GetAnimation() const
{
	const int32 AnimationParams = (bAnimateVertically ? SThrobber::Vertical : 0) |
		(bAnimateHorizontally ? SThrobber::Horizontal : 0) |
		(bAnimateOpacity ? SThrobber::Opacity : 0);

	return static_cast<SThrobber::EAnimation>(AnimationParams);
}

const FSlateBrush* UThrobber::GetPieceBrush() const
{
	if (PieceImage == NULL)
	{
		SThrobber::FArguments DefaultArgs;
		return DefaultArgs._PieceImage;
	}
	return &PieceImage->Brush;
}

void UThrobber::SetNumberOfPieces(int32 InNumberOfPieces)
{
	NumberOfPieces = NumberOfPieces;
	if (MyThrobber.IsValid())
	{
		MyThrobber->SetNumPieces(InNumberOfPieces);
	}
}

void UThrobber:: SetAnimateHorizontally(bool bInAnimateHorizontally)
{
	bInAnimateHorizontally = bInAnimateHorizontally;
	if (MyThrobber.IsValid())
	{
		MyThrobber->SetAnimate(GetAnimation());
	}
}

void UThrobber:: SetAnimateVertically(bool bInAnimateVertically)
{
	bInAnimateVertically = bInAnimateVertically;
	if (MyThrobber.IsValid())
	{
		MyThrobber->SetAnimate(GetAnimation());
	}
}

void UThrobber:: SetAnimateOpacity(bool bInAnimateOpacity)
{
	bInAnimateOpacity = bInAnimateOpacity;
	if (MyThrobber.IsValid())
	{
		MyThrobber->SetAnimate(GetAnimation());
	}
}

void UThrobber::SetPieceImage(USlateBrushAsset* InPieceImage)
{
	PieceImage = InPieceImage;
	if (MyThrobber.IsValid())
	{
		MyThrobber->SetPieceImage(GetPieceBrush());
	}
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
