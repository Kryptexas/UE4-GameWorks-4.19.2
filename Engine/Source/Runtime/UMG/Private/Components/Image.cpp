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

void UImage::ReleaseNativeWidget()
{
	Super::ReleaseNativeWidget();

	MyImage.Reset();
}

TSharedRef<SWidget> UImage::RebuildWidget()
{
	MyImage = SNew(SImage);
	return MyImage.ToSharedRef();
}

void UImage::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	TAttribute<FSlateColor> ColorAndOpacityBinding = OPTIONAL_BINDING(FSlateColor, ColorAndOpacity);
	TAttribute<const FSlateBrush*> ImageBinding = OPTIONAL_BINDING_CONVERT(USlateBrushAsset*, Image, const FSlateBrush*, ConvertImage);

	MyImage->SetImage(ImageBinding);
	MyImage->SetColorAndOpacity(ColorAndOpacityBinding);
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

const FSlateBrush* UImage::ConvertImage(TAttribute<USlateBrushAsset*> InImageAsset) const
{
	USlateBrushAsset* ImageAsset = InImageAsset.Get();
	if ( ImageAsset != Image )
	{
		UImage* MutableThis = const_cast< UImage* >( this );
		MutableThis->DynamicBrush = TOptional<FSlateBrush>();
		MutableThis->Image = ImageAsset;
	}
	else if ( DynamicBrush.IsSet() )
	{
		return &DynamicBrush.GetValue();
	}

	if ( Image == NULL && IsDesignTime() )
	{
		SImage::FArguments ImageDefaults;
		return ImageDefaults._Image.Get();
	}

	return Image ? &Image->Brush : NULL;
}

const FSlateBrush* UImage::GetImageBrush() const
{
	return ConvertImage(Image);
}

void UImage::SetImage(USlateBrushAsset* InImage)
{
	if ( InImage != Image )
	{
		DynamicBrush = TOptional<FSlateBrush>();
	}

	Image = InImage;
	if ( MyImage.IsValid() )
	{
		MyImage->SetImage(GetImageBrush());
	}
}

void UImage::SetImageFromBrush(FSlateBrush InImage)
{
	DynamicBrush = InImage;

	if ( MyImage.IsValid() )
	{
		MyImage->SetImage(&DynamicBrush.GetValue());
	}
}

void UImage::SetImageFromTexture(UTexture2D* Texture)
{
	FSlateBrush TextureBrush;
	TextureBrush.SetResourceObject(Texture);

	DynamicBrush = TextureBrush;

	if ( MyImage.IsValid() )
	{
		MyImage->SetImage(&DynamicBrush.GetValue());
	}
}

void UImage::SetImageFromMaterial(UMaterialInterface* Material)
{
	FSlateBrush MaterialBrush;
	MaterialBrush.SetResourceObject(Material);

	DynamicBrush = MaterialBrush;

	if ( MyImage.IsValid() )
	{
		MyImage->SetImage(&DynamicBrush.GetValue());
	}
}

UMaterialInstanceDynamic* UImage::GetDynamicMaterial()
{
	UMaterialInterface* Material = NULL;

	if ( !DynamicBrush.IsSet() )
	{
		const FSlateBrush* Brush = GetImageBrush();
		UObject* Resource = Brush->GetResourceObject();
		Material = Cast<UMaterialInterface>(Resource);
	}
	else
	{
		UObject* Resource = DynamicBrush.GetValue().GetResourceObject();
		Material = Cast<UMaterialInterface>(Resource);
	}

	if ( Material )
	{
		UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(Material);

		if ( !DynamicMaterial )
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(Material, this);

			const FSlateBrush* Brush = GetImageBrush();
			FSlateBrush ClonedBrush = *Brush;
			ClonedBrush.SetResourceObject(DynamicMaterial);

			DynamicBrush = ClonedBrush;

			if ( MyImage.IsValid() )
			{
				MyImage->SetImage(&DynamicBrush.GetValue());
			}
		}
		
		return DynamicMaterial;
	}

	//TODO UMG can we do something for textures?  General purpose dynamic material for them?

	return NULL;
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

const FText UImage::GetToolboxCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
