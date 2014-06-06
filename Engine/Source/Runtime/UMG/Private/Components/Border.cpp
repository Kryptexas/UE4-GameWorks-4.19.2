// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UBorder

UBorder::UBorder(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SBorder::FArguments BorderDefaults;

	HorizontalAlignment = BorderDefaults._HAlign;
	VerticalAlignment = BorderDefaults._VAlign;
	ContentPadding = BorderDefaults._Padding.Get();
	ContentScale = BorderDefaults._ContentScale.Get();
	ContentColorAndOpacity = FLinearColor::White;

	DesiredSizeScale = BorderDefaults._DesiredSizeScale.Get();
	BorderColorAndOpacity = FLinearColor::White;
	ForegroundColor = FLinearColor::Black;
	bShowEffectWhenDisabled = BorderDefaults._ShowEffectWhenDisabled.Get();
}

TSharedRef<SWidget> UBorder::RebuildWidget()
{
	MyBorder = SNew(SBorder);
	MyBorder->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);

	return MyBorder.ToSharedRef();
}

void UBorder::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	if ( MyBorder.IsValid() )
	{
		MyBorder->SetHAlign(HorizontalAlignment);
		MyBorder->SetVAlign(VerticalAlignment);
		MyBorder->SetPadding(ContentPadding);

		MyBorder->SetBorderBackgroundColor(BorderColorAndOpacity);
		MyBorder->SetColorAndOpacity(ContentColorAndOpacity);
		MyBorder->SetForegroundColor(ForegroundColor);

		MyBorder->SetContentScale(ContentScale);
		MyBorder->SetDesiredSizeScale(DesiredSizeScale);

		MyBorder->SetShowEffectWhenDisabled(bShowEffectWhenDisabled);

		MyBorder->SetBorderImage(GetBorderBrush());

		// SLATE_EVENT( FPointerEventHandler, OnMouseButtonDown )
		// SLATE_EVENT( FPointerEventHandler, OnMouseButtonUp )
		// SLATE_EVENT( FPointerEventHandler, OnMouseMove )
		// SLATE_EVENT( FPointerEventHandler, OnMouseDoubleClick )
	}
}

void UBorder::SetContent(UWidget* Content)
{
	Super::SetContent(Content);

	if ( MyBorder.IsValid() )
	{
		MyBorder->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);
	}
}

const FSlateBrush* UBorder::GetBorderBrush() const
{
	if ( BorderBrush == NULL )
	{
		SBorder::FArguments BorderDefaults;
		return BorderDefaults._BorderImage.Get();
	}

	return &BorderBrush->Brush;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
