// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UImageComponent

USpinningImageComponent::USpinningImageComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SSpinningImage::FArguments DefaultArgs;
	Period = DefaultArgs._Period;
}

TSharedRef<SWidget> USpinningImageComponent::RebuildWidget()
{
	return SNew(SSpinningImage)
		.Image( &Image )
		.ColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetColorAndOpacity) )
		.Period( Period );
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
