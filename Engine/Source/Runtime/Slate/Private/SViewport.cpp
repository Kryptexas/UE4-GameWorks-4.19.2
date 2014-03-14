// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

SViewport::SViewport()
	: ViewportInterface(NULL)
	, bRenderDirectlyToWindow(false)
	, bEnableGammaCorrection(true)
 {
 }

/**
 * Construct the widget.
 *
 * @param InArgs  Declaration from which to construct the widget.
 */
void SViewport::Construct(const FArguments& InArgs)
{
	ShowDisabledEffect = InArgs._ShowEffectWhenDisabled;
	bRenderDirectlyToWindow = InArgs._RenderDirectlyToWindow;
	bEnableGammaCorrection = InArgs._EnableGammaCorrection;
	bEnableBlending = InArgs._EnableBlending;
	bIgnoreTextureAlpha = InArgs._IgnoreTextureAlpha;
	CurrentlyThunkingMouseEventsAsTouchEvents = false;

	this->ChildSlot
	[
		InArgs._Content.Widget
	];
}


int32 SViewport::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
#if SLATE_HD_STATS
	SCOPE_CYCLE_COUNTER( STAT_SlateOnPaint_SViewport );
#endif
	bool bEnabled = ShouldBeEnabled( bParentEnabled );
	bool bShowDisabledEffect = ShowDisabledEffect.Get();
	ESlateDrawEffect::Type DrawEffects = bShowDisabledEffect && !bEnabled ? ESlateDrawEffect::DisabledEffect : ESlateDrawEffect::None;

	// Viewport texture alpha channels are often in an indeterminate state, even after the resolve,
	// so we'll tell the shader to not use the alpha channel when blending
	if( bIgnoreTextureAlpha )
	{
		DrawEffects |= ESlateDrawEffect::IgnoreTextureAlpha;
	}

	TSharedPtr<ISlateViewport> ViewportInterfacePin = ViewportInterface.Pin();

	// Tell the interface that we are drawing.
	if (ViewportInterfacePin.IsValid())
	{
		ViewportInterfacePin->OnDrawViewport( AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );
	}

	// Only draw a quad if not rendering directly to the backbuffer
	if( !ShouldRenderDirectly() )
	{
		if( ViewportInterfacePin.IsValid() && ViewportInterfacePin->GetViewportRenderTargetTexture() != NULL )
		{
			FSlateDrawElement::MakeViewport( OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), ViewportInterfacePin, MyClippingRect, bEnableGammaCorrection, bEnableBlending, DrawEffects, InWidgetStyle.GetColorAndOpacityTint() );
		}
		else
		{
			// Viewport isn't ready yet, so just draw a black box
			static FSlateColorBrush BlackBrush( FColor::Black );
			FSlateDrawElement::MakeBox( OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &BlackBrush, MyClippingRect, DrawEffects, BlackBrush.GetTint( InWidgetStyle ) );
		}
	}

	int32 Layer = SCompoundWidget::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bEnabled );

	if( ViewportInterfacePin.IsValid() && ViewportInterfacePin->IsSoftwareCursorVisible() )
	{
		const FVector2D CursorPosScreenSpace = FSlateApplication::Get().GetCursorPos();		
		// @todo Slate: why are we calling OnCursorQuery in here?
		FCursorReply Reply = ViewportInterfacePin->OnCursorQuery( AllottedGeometry,
			FPointerEvent(
				CursorPosScreenSpace,
				CursorPosScreenSpace,
				FVector2D::ZeroVector,
				TSet<FKey>(),
				FModifierKeysState(false, false, false, false, false, false) )
		 );
		EMouseCursor::Type CursorType = Reply.GetCursor();

		const FSlateBrush* Brush = FCoreStyle::Get().GetBrush(TEXT("SoftwareCursor_Grab"));
		if( CursorType == EMouseCursor::CardinalCross )
		{
			Brush = FCoreStyle::Get().GetBrush(TEXT("SoftwareCursor_CardinalCross"));
		}

		LayerId++;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry( ViewportInterfacePin->GetSoftwareCursorPosition() - ( Brush->ImageSize / 2 ), Brush->ImageSize ),
			Brush,
			MyClippingRect
		);
	}

	return Layer;
}

void SViewport::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if( ViewportInterface.IsValid() )
	{
		ViewportInterface.Pin()->Tick( InDeltaTime );
	}
}

FCursorReply SViewport::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnCursorQuery( MyGeometry, CursorEvent ) : FCursorReply::Unhandled();
}

FPointerEvent SViewport::MouseEventToTouchEvent(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D LastLocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetLastScreenSpacePosition());

	FPointerEvent TouchEvent(
		MouseEvent.GetUserIndex(),
		ETouchIndex::Touch1,
		LocalPos,
		LastLocalPos,
		true);

	return TouchEvent;
}

FReply SViewport::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (ShouldThunkMouseEventsAsTouchEvents())
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			const FReply Reply = OnTouchStarted(MyGeometry, MouseEventToTouchEvent(MyGeometry, MouseEvent));

			if (Reply.IsEventHandled())
			{
				return Reply;
			}
		}
	}

	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnMouseButtonDown( MyGeometry, MouseEvent ) : FReply::Unhandled();
}

FReply SViewport::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (ShouldThunkMouseEventsAsTouchEvents())
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			const FReply Reply = OnTouchEnded(MyGeometry, MouseEventToTouchEvent(MyGeometry, MouseEvent));

			if (Reply.IsEventHandled())
			{
				return Reply;
			}
		}
	}

	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnMouseButtonUp( MyGeometry, MouseEvent ) : FReply::Unhandled();
}

void SViewport::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

	if (ShouldThunkMouseEventsAsTouchEvents())
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			const FReply Reply = OnTouchStarted(MyGeometry, MouseEventToTouchEvent(MyGeometry, MouseEvent));

			if (Reply.IsEventHandled())
			{
				return;
			}
		}
	}

	if ( ViewportInterface.IsValid() )
	{
		ViewportInterface.Pin()->OnMouseEnter( MyGeometry, MouseEvent );
	}
}

void SViewport::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	if (ShouldThunkMouseEventsAsTouchEvents())
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			FGeometry MyGeometry;//@TODO: TouchEmulation: Need to adjust the signature of everything to add this
			const FReply Reply = OnTouchEnded(MyGeometry, MouseEventToTouchEvent(MyGeometry, MouseEvent));

			if (Reply.IsEventHandled())
			{
				return;
			}
		}
	}

	if ( ViewportInterface.IsValid() )
	{
		ViewportInterface.Pin()->OnMouseLeave( MouseEvent );
	}
}

FReply SViewport::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (ShouldThunkMouseEventsAsTouchEvents())
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			const FReply Reply = OnTouchMoved(MyGeometry, MouseEventToTouchEvent(MyGeometry, MouseEvent));

			if (Reply.IsEventHandled())
			{
				return Reply;
			}
		}
	}

	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnMouseMove( MyGeometry, MouseEvent ) : FReply::Unhandled();
}

FReply SViewport::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnMouseWheel( MyGeometry, MouseEvent ) : FReply::Unhandled();
}

FReply SViewport::OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnMouseButtonDoubleClick( MyGeometry, MouseEvent ) : FReply::Unhandled();
}

FReply SViewport::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& KeyboardEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnKeyDown( MyGeometry, KeyboardEvent ) : FReply::Unhandled();
}
 
FReply SViewport::OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& KeyboardEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnKeyUp( MyGeometry, KeyboardEvent ) : FReply::Unhandled();
}

FReply SViewport::OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnKeyChar( MyGeometry, CharacterEvent ) : FReply::Unhandled();
}

FReply SViewport::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	if(WidgetToFocusOnActivate.IsValid())
	{
		return FReply::Handled().SetKeyboardFocus(WidgetToFocusOnActivate.Pin().ToSharedRef(), InKeyboardFocusEvent.GetCause());
	}

	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnKeyboardFocusReceived( InKeyboardFocusEvent ) : FReply::Unhandled();
}

void SViewport::OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	if( ViewportInterface.IsValid() )
	{
		ViewportInterface.Pin()->OnKeyboardFocusLost( InKeyboardFocusEvent );
	}
}

void SViewport::OnWindowClosed( const TSharedRef<SWindow>& WindowBeingClosed )
{
	if( ViewportInterface.IsValid() )
	{
		ViewportInterface.Pin()->OnViewportClosed();
	}
}

FReply SViewport::OnControllerButtonPressed( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnControllerButtonPressed( MyGeometry, ControllerEvent ) : FReply::Unhandled();
}

FReply SViewport::OnControllerButtonReleased( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnControllerButtonReleased( MyGeometry, ControllerEvent ) : FReply::Unhandled();
}

FReply SViewport::OnControllerAnalogValueChanged( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnControllerAnalogValueChanged( MyGeometry, ControllerEvent ) : FReply::Unhandled();
}

FReply SViewport::OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnTouchStarted( MyGeometry, TouchEvent ) : FReply::Unhandled();
}

FReply SViewport::OnTouchMoved( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnTouchMoved( MyGeometry, TouchEvent ) : FReply::Unhandled();
}

FReply SViewport::OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnTouchEnded( MyGeometry, TouchEvent ) : FReply::Unhandled();
}

FReply SViewport::OnTouchGesture( const FGeometry& MyGeometry, const FPointerEvent& GestureEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnTouchGesture( MyGeometry, GestureEvent ) : FReply::Unhandled();
}

FReply SViewport::OnMotionDetected( const FGeometry& MyGeometry, const FMotionEvent& MotionEvent )
{
	return ViewportInterface.IsValid() ? ViewportInterface.Pin()->OnMotionDetected( MyGeometry, MotionEvent ) : FReply::Unhandled();
}
