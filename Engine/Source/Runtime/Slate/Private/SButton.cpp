// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"



/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SButton::Construct( const FArguments& InArgs )
{
	bIsPressed = false;

	// Text overrides button content. If nothing is specified, put an null widget in the button.
	// Null content makes the button enter a special mode where it will ask to be as big as the image used for its border.
	struct
	{
		TSharedRef<SWidget> operator()( const FArguments& InOpArgs ) const
		{
			if ((InOpArgs._Content.Widget == SNullWidget::NullWidget) && (InOpArgs._Text.IsBound() || !InOpArgs._Text.Get().IsEmpty()) )
			{
				return SNew(STextBlock)
					.Text( InOpArgs._Text )
					.TextStyle( InOpArgs._TextStyle );
			}
			else
			{
				return InOpArgs._Content.Widget;
			}
		}
	} DetermineContent; 

	SBorder::Construct( SBorder::FArguments()
		.ContentScale(InArgs._ContentScale)
		.DesiredSizeScale(InArgs._DesiredSizeScale)
		.BorderBackgroundColor(InArgs._ButtonColorAndOpacity)
		.ForegroundColor(InArgs._ForegroundColor)
		.BorderImage( this, &SButton::GetBorder )
		.HAlign( InArgs._HAlign )
		.VAlign( InArgs._VAlign )
		.Padding( TAttribute<FMargin>(this, &SButton::GetCombinedPadding) )
		[
			DetermineContent(InArgs)
		]
	);

	ContentPadding = InArgs._ContentPadding;

	/* Get pointer to the button style */
	const FButtonStyle* const ButtonStyle = InArgs._ButtonStyle;

	NormalImage = &ButtonStyle->Normal;
	HoverImage = &ButtonStyle->Hovered;
	PressedImage = &ButtonStyle->Pressed;

	BorderPadding = ButtonStyle->NormalPadding;
	PressedBorderPadding = ButtonStyle->PressedPadding;

	bIsFocusable = InArgs._IsFocusable;

	OnClicked = InArgs._OnClicked;

	ClickMethod = InArgs._ClickMethod;

	HoveredSound = InArgs._HoveredSoundOverride.Get(ButtonStyle->HoveredSlateSound);
	PressedSound = InArgs._PressedSoundOverride.Get(ButtonStyle->PressedSlateSound);
}

FMargin SButton::GetCombinedPadding() const
{
	return ( IsPressed() )
		? ContentPadding.Get() + PressedBorderPadding
		: ContentPadding.Get() + BorderPadding;	
}


/** @return An image that represents this button's border*/
const FSlateBrush* SButton::GetBorder() const
{
	if (IsPressed())
	{
		return PressedImage;
	}
	else if (IsHovered())
	{
		return HoverImage;
	}
	else
	{
		return NormalImage;
	}
}


bool SButton::SupportsKeyboardFocus() const
{
	// Buttons are focusable by default
	return bIsFocusable;
}

FReply SButton::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	//see if we pressed the Enter or Spacebar keys
	if(InKeyboardEvent.GetKey() == EKeys::Enter || InKeyboardEvent.GetKey() == EKeys::SpaceBar)
	{
		PlayPressedSound();

		//execute our "OnClicked" delegate, if we have one
		if(OnClicked.IsBound() == true)
		{
			return OnClicked.Execute();
		}
	}

	//if it was some other button, ignore it
	return FReply::Unhandled();
}


FReply SButton::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = FReply::Unhandled();
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		bIsPressed = true;
		PlayPressedSound();

		if( ClickMethod == EButtonClickMethod::MouseDown )
		{
			//get the reply from the execute function
			Reply = OnClicked.IsBound() ? OnClicked.Execute() : FReply::Handled();

			//You should ALWAYS handle the OnClicked event.
			ensure(Reply.IsEventHandled() == true);
		}
		else
		{
			//we need to capture the mouse for MouseUp events
			Reply =  FReply::Handled().CaptureMouse( AsShared() );
		}
	}

	//return the constructed reply
	return Reply;
}


FReply SButton::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	return OnMouseButtonDown( InMyGeometry, InMouseEvent );
}


FReply SButton::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	FReply Reply = FReply::Unhandled();
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		bIsPressed = false;

		if( ClickMethod == EButtonClickMethod::MouseDown )
		{
			// NOTE: If we're configured to click on mouse-down, then we never capture the mouse thus
			//       may never receive an OnMouseButtonUp() call.  We make sure that our bIsPressed
			//       state is reset by overriding OnMouseLeave().
		}
		else
		{
			const bool bIsUnderMouse = MyGeometry.IsUnderLocation(MouseEvent.GetScreenSpacePosition());
			if( bIsUnderMouse )
			{
				// If we were asked to allow the button to be clicked on mouse up, regardless of whether the user
				// pressed the button down first, then we'll allow the click to proceed without an active capture
				if( ( ClickMethod == EButtonClickMethod::MouseUp || HasMouseCapture() ) && OnClicked.IsBound() == true )
				{
					Reply = OnClicked.Execute();
					ensure(Reply.IsEventHandled() == true);
				}
			}
		}
		
		//If the user of the button didn't handle this click, then the button's
		//default behavior handles it.
		if(Reply.IsEventHandled() == false)
		{
			Reply = FReply::Handled();
		}

		//If the user hasn't requested a new mouse captor, then the default
		//behavior of the button is to release mouse capture.
		if(Reply.GetMouseCaptor().IsValid() == false)
		{
			Reply.ReleaseMouseCapture();
		}
	}

	return Reply;
}


FReply SButton::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

void SButton::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	PlayHoverSound();

	SBorder::OnMouseEnter( MyGeometry, MouseEvent );
}

void SButton::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	// Call parent implementation
	SWidget::OnMouseLeave( MouseEvent );

	// If we're setup to click on mouse-down, then we never capture the mouse and may not receive a
	// mouse up event, so we need to make sure our pressed state is reset properly here
	if( ClickMethod == EButtonClickMethod::MouseDown )
	{
		bIsPressed = false;
	}
}


void SButton::PlayPressedSound() const
{
	FSlateApplication::Get().PlaySound( PressedSound );
}

void SButton::PlayHoverSound() const
{
	FSlateApplication::Get().PlaySound( HoveredSound );
}

FVector2D SButton::ComputeDesiredSize() const
{
	// When there is no widget in the button, it sizes itself based on
	// the border image specified by the style.
	if (ChildSlot.Widget == SNullWidget::NullWidget)
	{
		return GetBorder()->ImageSize;
	}
	else
	{
		return SBorder::ComputeDesiredSize();
	}
}