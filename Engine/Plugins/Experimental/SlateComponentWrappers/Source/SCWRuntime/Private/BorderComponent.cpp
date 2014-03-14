// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateComponentWrappersPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SlateComponentWrappers"

/////////////////////////////////////////////////////
// UBorderComponent

UBorderComponent::UBorderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCanHaveMultipleChildren = false;

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
		.BorderImage(&BorderImage)
		.ContentScale(ContentScale)
		.DesiredSizeScale(DesiredSizeScale)
		.ColorAndOpacity(BIND_UOBJECT_ATTRIBUTE(FLinearColor, GetContentColor))
		.BorderBackgroundColor(BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetBorderColor))
		.ForegroundColor(BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetForegroundColor))
		.ShowEffectWhenDisabled(bShowEffectWhenDisabled);

	MyBorder = NewBorder;
	return NewBorder;
}

void UBorderComponent::OnKnownChildrenChanged()
{
	TSharedPtr<SBorder> Border = MyBorder.Pin();
	if (Border.IsValid())
	{
		Border->SetContent(GetFirstChildWidget());
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
