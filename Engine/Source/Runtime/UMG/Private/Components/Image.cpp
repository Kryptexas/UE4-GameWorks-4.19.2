// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UImage

UImage::UImage(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Image()
	, ColorAndOpacity(FLinearColor::White)
{
}

TSharedRef<SWidget> UImage::RebuildWidget()
{
	MyImage = SNew(SImage);
	return MyImage.ToSharedRef();
}

void UImage::SyncronizeProperties()
{
	MyImage->SetImage(GetImageBrush());
	MyImage->SetColorAndOpacity(ColorAndOpacity);
	MyImage->SetOnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonDown));
}

void UImage::SetColorAndOpacity(FLinearColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	if ( MyImage.IsValid() )
	{
		MyImage->SetColorAndOpacity(ColorAndOpacity);
	}
}

void UImage::SetImage(USlateBrushAsset* InImage)
{
	Image = InImage;
	if ( MyImage.IsValid() )
	{
		MyImage->SetImage(GetImageBrush());
	}
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

FReply UImage::HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	if ( OnMouseButtonDownEvent.IsBound() )
	{
		return OnMouseButtonDownEvent.Execute(Geometry, MouseEvent).ToReply(MyImage.ToSharedRef());
	}

	return FReply::Unhandled();
}

#if WITH_EDITOR

const FSlateBrush* UImage::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Image");
}

#endif


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
