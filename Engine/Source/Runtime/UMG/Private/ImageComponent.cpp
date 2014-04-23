// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UImageComponent

UImageComponent::UImageComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Image()
	, ColorAndOpacity(FLinearColor::White)
{
	//@TODO: Would like to default to the checkerboard, but TextureName always wins over TextureObject :(
//	Image = *FEditorStyle::GetDefaultBrush();
}

TSharedRef<SWidget> UImageComponent::RebuildWidget()
{
	return SNew(SImage)
		.Image( &Image )
		.ColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetColorAndOpacity) );
}

FSlateColor UImageComponent::GetColorAndOpacity() const
{
	return ColorAndOpacity;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
