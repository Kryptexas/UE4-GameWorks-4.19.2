// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UBorderComponent

UBorderComponent::UBorderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bIsVariable = false;

	SBorder::FArguments BorderDefaults;

	HorizontalAlignment = BorderDefaults._HAlign;
	VerticalAlignment = BorderDefaults._VAlign;
	ContentPadding = BorderDefaults._Padding.Get();
	//BorderImage = *BorderDefaults._BorderImage.Get();
	DesiredSizeScale = BorderDefaults._DesiredSizeScale.Get();
	ContentScale = BorderDefaults._ContentScale.Get();
	ContentColorAndOpacity = BorderDefaults._ColorAndOpacity.Get();
	BorderColorAndOpacity = BorderDefaults._BorderBackgroundColor.Get();
	ForegroundColor = BorderDefaults._ForegroundColor.Get();
	bShowEffectWhenDisabled = BorderDefaults._ShowEffectWhenDisabled.Get();
}

TSharedRef<SWidget> UBorderComponent::RebuildWidget()
{
	TSharedRef<SBorder> NewBorder = SNew(SBorder)
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		.Padding(BIND_UOBJECT_ATTRIBUTE(FMargin, GetContentPadding))
		// SLATE_EVENT( FPointerEventHandler, OnMouseButtonDown )
		// SLATE_EVENT( FPointerEventHandler, OnMouseButtonUp )
		// SLATE_EVENT( FPointerEventHandler, OnMouseMove )
		// SLATE_EVENT( FPointerEventHandler, OnMouseDoubleClick )
		.BorderImage(BIND_UOBJECT_ATTRIBUTE(const FSlateBrush*, GetBorderBrush))
		.ContentScale(ContentScale)
		.DesiredSizeScale(DesiredSizeScale)
		.ColorAndOpacity(BIND_UOBJECT_ATTRIBUTE(FLinearColor, GetContentColor))
		.BorderBackgroundColor(BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetBorderColor))
		.ForegroundColor(BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetForegroundColor))
		.ShowEffectWhenDisabled(bShowEffectWhenDisabled);

	NewBorder->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);

	MyBorder = NewBorder;
	return NewBorder;
}

void UBorderComponent::SetContent(USlateWrapperComponent* Content)
{
	Super::SetContent(Content);

	TSharedPtr<SBorder> Border = MyBorder.Pin();
	if ( Border.IsValid() )
	{
		Border->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);
	}
}

FMargin UBorderComponent::GetContentPadding() const
{
	return ContentPadding;
}

FLinearColor UBorderComponent::GetContentColor() const
{
	return ContentColorAndOpacity;
}

const FSlateBrush* UBorderComponent::GetBorderBrush() const
{
	if ( BorderBrush == NULL )
	{
		SBorder::FArguments BorderDefaults;
		return BorderDefaults._BorderImage.Get();
	}

	return &BorderBrush->Brush;
}

FSlateColor UBorderComponent::GetBorderColor() const
{
	return BorderColorAndOpacity;
}

FSlateColor UBorderComponent::GetForegroundColor() const
{
	return ForegroundColor;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
