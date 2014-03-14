// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UThrobberComponent

UThrobberComponent::UThrobberComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SThrobber::FArguments DefaultArgs;

	PieceImage = *DefaultArgs._PieceImage;
	NumberOfPieces = DefaultArgs._NumPieces;

	bAnimateVertically = (DefaultArgs._Animate & SThrobber::Vertical) != 0;
	bAnimateHorizontally = (DefaultArgs._Animate & SThrobber::Horizontal) != 0;
	bAnimateOpacity = (DefaultArgs._Animate & SThrobber::Opacity) != 0;
}

TSharedRef<SWidget> UThrobberComponent::RebuildWidget()
{
	const int32 AnimationParams = (bAnimateVertically ? SThrobber::Vertical : 0) |
		(bAnimateHorizontally ? SThrobber::Horizontal : 0) |
		(bAnimateOpacity ? SThrobber::Opacity : 0);

	return SNew(SThrobber)
		.PieceImage(&PieceImage)
		.NumPieces(NumberOfPieces)
		.Animate(static_cast<SThrobber::EAnimation>(AnimationParams));
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
