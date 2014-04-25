// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UButtonComponent

UButtonComponent::UButtonComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SButton::FArguments ButtonDefaults;

	Style = NULL;
	HorizontalAlignment = ButtonDefaults._HAlign;
	VerticalAlignment = ButtonDefaults._VAlign;
	ContentPadding = ButtonDefaults._ContentPadding.Get();

	DesiredSizeScale = ButtonDefaults._DesiredSizeScale.Get();
	ContentScale = ButtonDefaults._ContentScale.Get();

	ButtonColorAndOpacity = FLinearColor::White;
	ForegroundColor = FLinearColor::Black;
}

TSharedRef<SWidget> UButtonComponent::RebuildWidget()
{
	SButton::FArguments ButtonDefaults;

	const FButtonStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FButtonStyle>() : NULL;
	if (StylePtr == NULL)
	{
		StylePtr = ButtonDefaults._ButtonStyle;
	}

	TSharedRef<SButton> NewButton = SNew(SButton)
		.ButtonStyle(StylePtr)
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		.ContentPadding(BIND_UOBJECT_ATTRIBUTE(FMargin, GetContentPadding))
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateOnClickedCallback))
		.DesiredSizeScale(DesiredSizeScale)
		.ContentScale(ContentScale)
		.ButtonColorAndOpacity(BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetButtonColor))
		.ForegroundColor(BIND_UOBJECT_ATTRIBUTE(FSlateColor, GetForegroundColor));

	NewButton->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);
	
	MyButton = NewButton;
	return NewButton;
}

void UButtonComponent::SetContent(USlateWrapperComponent* InContent)
{
	Super::SetContent(InContent);

	TSharedPtr<SButton> Button = MyButton.Pin();
	if (Button.IsValid())
	{
		Button->SetContent(InContent ? InContent->GetWidget() : SNullWidget::NullWidget);
	}
}

FMargin UButtonComponent::GetContentPadding() const
{
	return ContentPadding;
}

FReply UButtonComponent::SlateOnClickedCallback()
{
	OnClicked.Broadcast();
	return FReply::Handled();
}

FSlateColor UButtonComponent::GetButtonColor() const
{
	return ButtonColorAndOpacity;
}

FSlateColor UButtonComponent::GetForegroundColor() const
{
	return ForegroundColor;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
