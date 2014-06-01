// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UImage

UImage::UImage(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Image()
	, ColorAndOpacity(FLinearColor::White)
{
	//@TODO: Would like to default to the checkerboard, but TextureName always wins over TextureObject :(
	//	Image = *FEditorStyle::GetDefaultBrush();
}

TSharedRef<SWidget> UImage::RebuildWidget()
{
	return SNew(SImage)
		.Image( BIND_UOBJECT_ATTRIBUTE(const FSlateBrush*, GetImageBrush) )
		.ColorAndOpacity( BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetColorAndOpacity) );
}

const FSlateBrush* UImage::GetImageBrush() const
{
	if ( Image == NULL )
	{
		SImage::FArguments ImageDefaults;
		return ImageDefaults._Image.Get();
	}

	return &Image->Brush;
}

FSlateColor UImage::GetColorAndOpacity() const
{
	return ColorAndOpacity;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
