// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UButton

UButton::UButton(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	HorizontalAlignment = HAlign_Center;
	VerticalAlignment = VAlign_Center;

	SButton::FArguments ButtonDefaults;

	Style = NULL;
	ContentPadding = ButtonDefaults._ContentPadding.Get();

	DesiredSizeScale = ButtonDefaults._DesiredSizeScale.Get();
	ContentScale = ButtonDefaults._ContentScale.Get();

	ButtonColorAndOpacity = FLinearColor::White;
	ForegroundColor = FLinearColor::Black;
}

TSharedRef<SWidget> UButton::RebuildWidget()
{
	MyButton = SNew(SButton);
	MyButton->SetContent(Content ? Content->GetWidget() : SNullWidget::NullWidget);
	
	return MyButton.ToSharedRef();
}

void UButton::SyncronizeProperties()
{
	Super::SyncronizeProperties();

	SButton::FArguments ButtonDefaults;

	const FButtonStyle* StylePtr = ( Style != NULL ) ? Style->GetStyle<FButtonStyle>() : NULL;
	if ( StylePtr == NULL )
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

	if ( MyButton.IsValid() )
	{
		MyButton->SetButtonStyle(StylePtr);
		MyButton->SetHAlign(HorizontalAlignment);
		MyButton->SetVAlign(VerticalAlignment);
		MyButton->SetContentPadding(ContentPadding);
		MyButton->SetForegroundColor(ForegroundColor);
		//MyButton->SetColorAndOpacity(ButtonColorAndOpacity);
		MyButton->SetDesiredSizeScale(DesiredSizeScale);
		MyButton->SetContentScale(ContentScale);
		MyButton->SetBorderBackgroundColor(ButtonColorAndOpacity);
		MyButton->SetPressedSound(OptionalPressedSound);
		MyButton->SetHoveredSound(OptionalHoveredSound);
		MyButton->SetOnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleOnClicked));
	}
}

void UButton::SetContent(UWidget* InContent)
{
	Super::SetContent(InContent);

	if ( MyButton.IsValid() )
	{
		MyButton->SetContent(InContent ? InContent->GetWidget() : SNullWidget::NullWidget);
	}
}

void UButton::SetButtonColorAndOpacity(FLinearColor InButtonColorAndOpacity)
{
	MyButton->SetBorderBackgroundColor(InButtonColorAndOpacity);
}

void UButton::SetForegroundColor(FLinearColor InForegroundColor)
{
	MyButton->SetForegroundColor(InForegroundColor);
}

void UButton::SetContentPadding(FMargin InContentPadding)
{
	MyButton->SetContentPadding(InContentPadding);
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
