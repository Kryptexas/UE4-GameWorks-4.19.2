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

void UImage::SetOpacity(float InOpacity)
{
	ColorAndOpacity.A = InOpacity;
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

UMaterialInstanceDynamic* UImage::GetDynamicMaterial()
{
	if ( !DynamicBrush.IsSet() )
	{
		const FSlateBrush* Brush = UImage::GetImageBrush();
		FSlateBrush ClonedBrush = *Brush;
		UObject* Resource = ClonedBrush.GetResourceObject();
		UMaterialInterface* Material = Cast<UMaterialInterface>(Resource);

		if ( Material )
		{
			UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);
			ClonedBrush.SetResourceObject(DynamicMaterial);
			
			DynamicBrush = ClonedBrush;

			if ( MyImage.IsValid() )
			{
				MyImage->SetImage(&DynamicBrush.GetValue());
			}
		}
		else
		{
			//TODO UMG Log error for debugging.
		}
	}
	else
	{
		UObject* Resource = DynamicBrush.GetValue().GetResourceObject();
		return Cast<UMaterialInstanceDynamic>(Resource);
	}

	return NULL;
}

const FSlateBrush* UImage::GetImageBrush() const
{
	if ( DynamicBrush.IsSet() )
	{
		return &DynamicBrush.GetValue();
	}

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
