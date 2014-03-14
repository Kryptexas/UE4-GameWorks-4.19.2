// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
 #include "Slate.h"

/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SMenuAnchor::Construct( const FArguments& InArgs )
{
	PopupLayerSlot = NULL;

	this->ChildSlot
	.Padding(0)
	[
		InArgs._Content.Widget
	];
	MenuContent = InArgs._MenuContent;
	OnGetMenuContent = InArgs._OnGetMenuContent;
	Placement = InArgs._Placement;
	Method = InArgs._Method;
}

void SMenuAnchor::ComputeMenuPlacement( const FGeometry& AllottedGeometry, const FVector2D& PopupDesiredSize, FVector2D& NewWindowSize, FVector2D& NewPosition )
{
	const EMenuPlacement PlacementMode = Placement.Get();

	if ( PlacementMode == MenuPlacement_ComboBox )
	{
		NewWindowSize = FVector2D( FMath::Max(AllottedGeometry.Size.X, PopupDesiredSize.X), PopupDesiredSize.Y );
		NewWindowSize *= AllottedGeometry.Scale;
	}

	const float AbsoluteX = AllottedGeometry.AbsolutePosition.X;
	const float AbsoluteY = AllottedGeometry.AbsolutePosition.Y;
	FSlateRect Anchor(AbsoluteX, AbsoluteY, AbsoluteX + AllottedGeometry.Size.X*AllottedGeometry.Scale, AbsoluteY + AllottedGeometry.Size.Y*AllottedGeometry.Scale);
	EOrientation Orientation = (PlacementMode == MenuPlacement_MenuRight) ? Orient_Horizontal : Orient_Vertical;
	NewPosition = FSlateApplication::Get().CalculatePopupWindowPosition( Anchor, NewWindowSize, Orientation );
}

void SMenuAnchor::RequestClosePopupWindow( const TSharedRef<SWindow>& PopupWindow )
{
	bDismissedThisTick = true;
	if (ensure(Method == CreateNewWindow))
	{
		FSlateApplication::Get().RequestDestroyWindow(PopupWindow);
	}
}

void SMenuAnchor::OnClickedOutsidePopup()
{
	bDismissedThisTick = true;
	if (ensure(Method == UseCurrentWindow))
	{
		FSlateApplication::Get().GetPopupSupport().UnregisterClickNotification( FOnClickedOutside::CreateSP(this, &SMenuAnchor::OnClickedOutsidePopup) );
		SetIsOpen(false);	
	}
}

void SMenuAnchor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TSharedPtr<SWindow> PopupWindow = PopupWindowPtr.Pin();
	if ( PopupWindow.IsValid() )
	{
		// Figure out where our attached pop-up window should be placed.				
		if (Method == CreateNewWindow)
		{
			FVector2D NewPosition = FVector2D::ZeroVector;
			FVector2D NewSize = PopupWindow->GetSizeInScreen();
			ComputeMenuPlacement( AllottedGeometry, PopupWindow->GetContent()->GetDesiredSize(), NewSize, NewPosition );

			// We made a window for showing the popup.
			// Update the window's position!
			if( PopupWindow->IsMorphing() )
			{
				FSlateRect NewShape = FSlateRect(NewPosition.X, NewPosition.Y, NewPosition.X + NewSize.X, NewPosition.Y + NewSize.Y);
				if( NewShape != PopupWindow->GetMorphTargetShape() )
				{
					// Update the target shape
					PopupWindow->UpdateMorphTargetShape( NewShape );
					// Set size immediately if not morphing size
					if(!PopupWindow->IsMorphingSize())
					{
						PopupWindow->ReshapeWindow( PopupWindow->GetPositionInScreen(), NewSize );
					}
				}
			}
			else
			{
				if( NewPosition != PopupWindow->GetPositionInScreen() || NewSize != PopupWindow->GetSizeInScreen() )
				{
					PopupWindow->ReshapeWindow( NewPosition, NewSize );
				}
			}
		}
		else
		{
			// Using the PopupLayer to show this popup instead of a window.
			// Update the Popup's position so it appears anchored to the menu anchor.
			if (ensure(PopupLayerSlot != NULL))
			{
				FVector2D NewPosition = FVector2D::ZeroVector;
				FVector2D NewSize = PopupLayerSlot->GetWidget()->GetDesiredSize();

				ComputeMenuPlacement( AllottedGeometry, PopupLayerSlot->GetWidget()->GetDesiredSize(), NewSize, NewPosition );
			
				(*PopupLayerSlot)
					.DesktopPosition( NewPosition )
					.WidthOverride( NewSize.X )
					.Scale(AllottedGeometry.Scale);
			}
		}
	}

	/** The tick is ending, so the window was not dismissed this tick. */
	bDismissedThisTick = false;
}

/**
 * Open or close the popup
 *
 * @param InIsOpen  If true, open the popup. Otherwise close it.
 */
void SMenuAnchor::SetIsOpen( bool InIsOpen, const bool bFocusMenu )
{
	// Prevent redundant opens/closes
	if ( IsOpen() != InIsOpen )
	{
		TSharedPtr< SWidget > MenuContentPtr = OnGetMenuContent.IsBound() ? OnGetMenuContent.Execute() : MenuContent;

		if (InIsOpen && MenuContentPtr.IsValid() )
		{
			// OPEN POPUP
			
			// This can be called at any time so we use the push menu override that explicitly allows us to specify our parent

			// Figure out where the menu anchor is on the screen, so we can set the initial position of our pop-up window
			SlatePrepass();

			// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
			FWidgetPath MyWidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetChecked( AsShared(), MyWidgetPath );
			const FGeometry& MyGeometry = MyWidgetPath.Widgets.Last().Geometry;

			// Figure out how big the content widget is so we can set the window's initial size properly
			TSharedRef< SWidget > MenuContentRef = MenuContentPtr.ToSharedRef();
			MenuContentRef->SlatePrepass();


			// Combo-boxes never size down smaller than the widget that spawned them, but all
			// other pop-up menus are currently auto-sized
			const FVector2D DesiredContentSize = MenuContentRef->GetDesiredSize();  // @todo: This is ignoring any window border size!
			const EMenuPlacement PlacementMode = Placement.Get();
			bool bShouldAutoSize = PlacementMode != MenuPlacement_ComboBox;

			const FVector2D NewPosition = MyGeometry.AbsolutePosition;
			FVector2D NewWindowSize = DesiredContentSize;
			const FVector2D SummonLocationSize = MyGeometry.Size;

			FPopupTransitionEffect TransitionEffect( FPopupTransitionEffect::None );
			if( PlacementMode == MenuPlacement_ComboBox )
			{
				TransitionEffect = FPopupTransitionEffect( FPopupTransitionEffect::ComboButton );
				NewWindowSize = FVector2D( FMath::Max(MyGeometry.Size.X, DesiredContentSize.X), DesiredContentSize.Y );
			}
			else if( PlacementMode == MenuPlacement_BelowAnchor )
			{
				TransitionEffect = FPopupTransitionEffect( FPopupTransitionEffect::TopMenu );
			}
			else if( PlacementMode == MenuPlacement_MenuRight )
			{
				TransitionEffect = FPopupTransitionEffect( FPopupTransitionEffect::SubMenu );
			}

			if (Method == CreateNewWindow)
			{
				// Open the pop-up
				TSharedRef<SWindow> PopupWindow = FSlateApplication::Get().PushMenu( AsShared(), MenuContentRef, NewPosition, TransitionEffect, bFocusMenu, bShouldAutoSize, NewWindowSize, SummonLocationSize );
				PopupWindow->SetRequestDestroyWindowOverride( FRequestDestroyWindowOverride::CreateSP(this, &SMenuAnchor::RequestClosePopupWindow) );
				PopupWindowPtr = PopupWindow;
			}
			else
			{
				// We are re-using the current window instead of creating a new one.
				// The popup will be presented via an overlay service.
				ensure(Method==UseCurrentWindow);
				TSharedRef<SWindow> PopupWindow = MyWidgetPath.GetWindow();
				PopupWindowPtr = PopupWindow;
				
				TSharedPtr<SWeakWidget> PopupWrapperPtr;
				PopupLayerSlot = &PopupWindow->AddPopupLayerSlot()
				.DesktopPosition(NewPosition)
				.WidthOverride(NewWindowSize.X)
				[
					SAssignNew( PopupWrapperPtr, SWeakWidget )
					.PossiblyNullContent
					(
						MenuContentRef
					)
				];

				// We want to support dismissing the popup widget when the user clicks outside it.
				FSlateApplication::Get().GetPopupSupport().RegisterClickNotification( PopupWrapperPtr.ToSharedRef(), FOnClickedOutside::CreateSP(this, &SMenuAnchor::OnClickedOutsidePopup) );
			}

			if(bFocusMenu)
			{
				FSlateApplication::Get().SetKeyboardFocus(MenuContentRef, EKeyboardFocusCause::SetDirectly);
			}
		}
		else
		{
			// CLOSE POPUP

			if (Method == CreateNewWindow)
			{
				// Close the Popup window.
				TSharedPtr<SWindow> PopupWindow = PopupWindowPtr.Pin();
				if ( PopupWindow.IsValid() )
				{
					// Request that the popup be closed.
					PopupWindow->RequestDestroyWindow();
				}
			}
			else
			{
				// Close the popup overlay
				ensure(Method==UseCurrentWindow);
				check(PopupWindowPtr.IsValid());
				check(PopupLayerSlot != NULL);
				PopupWindowPtr.Pin()->RemovePopupLayerSlot(PopupLayerSlot->GetWidget());
				// We no longer have a popup open, so reset all the tracking state associated.
				PopupLayerSlot = NULL;
				PopupWindowPtr.Reset();
			}
		}
	}
}

bool SMenuAnchor::IsOpen() const
{
	return PopupWindowPtr.Pin().IsValid();
}

bool SMenuAnchor::ShouldOpenDueToClick() const
{
	return !IsOpen() && !bDismissedThisTick;
}

FVector2D SMenuAnchor::GetMenuPosition() const
{
	FVector2D Pos(0,0);
	if( PopupWindowPtr.IsValid() )
	{
		Pos = PopupWindowPtr.Pin()->GetPositionInScreen();
	}
	
	return Pos;
}

bool SMenuAnchor::HasOpenSubMenus() const
{
	bool Result = false;
	if( PopupWindowPtr.IsValid() )
	{
		Result = FSlateApplication::Get().HasOpenSubMenus( PopupWindowPtr.Pin().ToSharedRef() );
	}
	return Result;
}

SMenuAnchor::SMenuAnchor()
	: MenuContent( SNullWidget::NullWidget )
	, bDismissedThisTick( false )
	, Method(CreateNewWindow)
{
}