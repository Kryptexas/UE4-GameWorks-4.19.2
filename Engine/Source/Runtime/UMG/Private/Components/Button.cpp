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

	DesiredSizeScale = ButtonDefaults._DesiredSizeScale.Get();
	ContentScale = ButtonDefaults._ContentScale.Get();

	ColorAndOpacity = FLinearColor::White;
	BackgroundColor = FLinearColor::White;
	ForegroundColor = FLinearColor::Black;
}

TSharedRef<SWidget> UButton::RebuildWidget()
{
	MyButton = SNew(SButton);

	if ( GetChildrenCount() > 0 )
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}
	
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

	MyButton->SetButtonStyle( StylePtr );

	MyButton->SetForegroundColor( ForegroundColor );
	MyButton->SetColorAndOpacity( ColorAndOpacity );
	MyButton->SetBorderBackgroundColor( BackgroundColor );
	
	MyButton->SetDesiredSizeScale( DesiredSizeScale );
	MyButton->SetContentScale( ContentScale );
	MyButton->SetPressedSound( OptionalPressedSound );
	MyButton->SetHoveredSound( OptionalHoveredSound );

	MyButton->SetOnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, HandleOnClicked));
}

UClass* UButton::GetSlotClass() const
{
	return UButtonSlot::StaticClass();
}

void UButton::OnSlotAdded(UPanelSlot* Slot)
{
	// Add the child to the live slot if it already exists
	if ( MyButton.IsValid() )
	{
		Cast<UButtonSlot>(Slot)->BuildSlot(MyButton.ToSharedRef());
	}
}

void UButton::OnSlotRemoved(UPanelSlot* Slot)
{
	// Remove the widget from the live slot if it exists.
	if ( MyButton.IsValid() )
	{
		MyButton->SetContent(SNullWidget::NullWidget);
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

void UButton::PostLoad()
{
	Super::PostLoad();

	if ( GetChildrenCount() > 0 )
	{
		//TODO UMG Pre-Release Upgrade, now buttons have slots of their own.  Convert existing slot to new slot.
		UButtonSlot* ButtonSlot = Cast<UButtonSlot>(GetContentSlot());
		if ( ButtonSlot == NULL )
		{
			ButtonSlot = ConstructObject<UButtonSlot>(UButtonSlot::StaticClass(), this);
			ButtonSlot->Content = GetContentSlot()->Content;
			ButtonSlot->Content->Slot = ButtonSlot;
			Slots[0] = ButtonSlot;
		}
	}
}

#if WITH_EDITOR

const FSlateBrush* UButton::GetEditorIcon()
{
	return FUMGStyle::Get().GetBrush("Widget.Button");
}

#endif

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
