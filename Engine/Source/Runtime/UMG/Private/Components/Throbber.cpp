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

	const FSlateBrush* Image = PieceImage ? &PieceImage->Brush : DefaultArgs._PieceImage;

	const int32 AnimationParams = (bAnimateVertically ? SThrobber::Vertical : 0) |
		(bAnimateHorizontally ? SThrobber::Horizontal : 0) |
		(bAnimateOpacity ? SThrobber::Opacity : 0);

	return SNew(SThrobber)
		.PieceImage(Image)
		.NumPieces(NumberOfPieces)
		.Animate(static_cast<SThrobber::EAnimation>(AnimationParams));
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
