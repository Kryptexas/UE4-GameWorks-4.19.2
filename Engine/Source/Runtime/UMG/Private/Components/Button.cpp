// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UButton

UButton::UButton(const FPostConstructInitializeProperties& PCIP)
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

TSharedRef<SWidget> UButton::RebuildWidget()
{
	SButton::FArguments ButtonDefaults;

	const FButtonStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FButtonStyle>() : NULL;
	if (StylePtr == NULL)
	{
		StylePtr = ButtonDefaults._ButtonStyle;
	}

	TOptional<FSlateSound> OptionalPressedSound;
	if ( PressedSound.GetResourceObject() )
	{
		OptionalPressedSound = PressedSound;
	}

	TOptional<FSlateSound> OptionalHoveredSound;
	if ( HoveredSound.GetResourceObject() )
	{
		OptionalHoveredSound = HoveredSound;
	}

	TSharedRef<SButton> NewButton = SNew(SButton)
		.ButtonStyle(StylePtr)
		.HAlign(HorizontalAlignment)
		.VAlign(VerticalAlignment)
		.ContentPadding(ContentPadding)
		.DesiredSizeScale(DesiredSizeScale)
		.ContentScale(ContentScale)
		.ButtonColorAndOpacity(ButtonColorAndOpacity)
		.ForegroundColor(ForegroundColor)
		.PressedSoundOverride(OptionalPressedSound)
		.HoveredSoundOverride(OptionalHoveredSound)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleOnClicked))
		;

	NewButton->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);
	
	return NewButton;
}

void UButton::SetContent(UWidget* InContent)
{
	Super::SetContent(InContent);

	if ( ButtonWidget().IsValid() )
	{
		ButtonWidget()->SetContent(InContent ? InContent->GetWidget() : SNullWidget::NullWidget);
	}
}

FLinearColor UButton::GetButtonColorAndOpacity()
{
	return ButtonWidget()->GetBorderBackgroundColor().GetSpecifiedColor();
}

void UButton::SetButtonColorAndOpacity(FLinearColor InButtonColorAndOpacity)
{
	ButtonWidget()->SetBorderBackgroundColor(InButtonColorAndOpacity);
}

FLinearColor UButton::GetForegroundColor()
{
	return ButtonWidget()->GetForegroundColor().GetSpecifiedColor();
}

void UButton::SetForegroundColor(FLinearColor InForegroundColor)
{
	ButtonWidget()->SetForegroundColor(InForegroundColor);
}

FMargin UButton::GetContentPadding()
{
	return ButtonWidget()->GetContentPadding();
}

void UButton::SetContentPadding(FMargin InContentPadding)
{
	ButtonWidget()->SetContentPadding(InContentPadding);
}

FReply UButton::HandleOnClicked()
{
	if ( OnClickedEvent.IsBound() )
	{
		return OnClickedEvent.Execute().ToReply();
	}
	
	return FReply::Unhandled();
}


/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
