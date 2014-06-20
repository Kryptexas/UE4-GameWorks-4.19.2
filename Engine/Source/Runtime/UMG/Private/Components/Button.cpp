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

	ColorAndOpacity = FLinearColor::White;
	BackgroundColor = FLinearColor::White;
	ForegroundColor = FLinearColor::Black;
}

TSharedRef<SWidget> UButton::RebuildWidget()
{
	MyButton = SNew(SButton);
	MyButton->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->GetWidget() : SNullWidget::NullWidget);
	
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

	MyButton->SetButtonStyle(StylePtr);
	MyButton->SetHAlign(HorizontalAlignment);
	MyButton->SetVAlign(VerticalAlignment);
	MyButton->SetContentPadding(ContentPadding);
	
	MyButton->SetForegroundColor( ForegroundColor );
	MyButton->SetColorAndOpacity( ColorAndOpacity );
	MyButton->SetBorderBackgroundColor( BackgroundColor );
	
	MyButton->SetDesiredSizeScale(DesiredSizeScale);
	MyButton->SetContentScale(ContentScale);
	MyButton->SetPressedSound(OptionalPressedSound);
	MyButton->SetHoveredSound(OptionalHoveredSound);

	MyButton->SetOnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleOnClicked));
}

void UButton::SetContent(UWidget* InContent)
{
	Super::SetContent(InContent);

	if ( MyButton.IsValid() )
	{
		MyButton->SetContent(InContent ? InContent->GetWidget() : SNullWidget::NullWidget);
	}
}

void UButton::SetColorAndOpacity(FLinearColor Color)
{
	ColorAndOpacity = Color;
	if ( MyButton.IsValid() )
	{
		MyButton->SetColorAndOpacity(Color);
	}
}

void UButton::SetBackgroundColor(FLinearColor Color)
{
	BackgroundColor = Color;
	if ( MyButton.IsValid() )
	{
		MyButton->SetBorderBackgroundColor(Color);
	}
}

void UButton::SetForegroundColor(FLinearColor InForegroundColor)
{
	ForegroundColor = InForegroundColor;
	if ( MyButton.IsValid() )
	{
		MyButton->SetForegroundColor(InForegroundColor);
	}
}

void UButton::SetContentPadding(FMargin InContentPadding)
{
	ContentPadding = InContentPadding;
	if ( MyButton.IsValid() )
	{
		MyButton->SetContentPadding(InContentPadding);
	}
}

FReply UButton::HandleOnClicked()
{
	if ( OnClickedEvent.IsBound() )
	{
		return OnClickedEvent.Execute().ToReply( MyButton.ToSharedRef() );
	}
	
	return FReply::Unhandled();
}

bool UButton::IsPressed() const
{
	return MyButton->IsPressed();
}

#if WITH_EDITOR

const FSlateBrush* UButton::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Button");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
