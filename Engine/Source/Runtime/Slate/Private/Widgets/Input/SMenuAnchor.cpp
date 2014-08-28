// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 
#include "SlatePrivatePCH.h"


/**
 * Construct this widget
 *
 * @param	InArgs	The declaration data for this widget
 */
void SMenuAnchor::Construct( const FArguments& InArgs )
{
	Children.Add( new FSimpleSlot() );
	Children.Add( new FSimpleSlot() );
	

	Children[0]
	.Padding(0)
	[
		InArgs._Content.Widget
	];
	MenuContent = InArgs._MenuContent;
	OnGetMenuContent = InArgs._OnGetMenuContent;
	OnMenuOpenChanged = InArgs._OnMenuOpenChanged;
	Placement = InArgs._Placement;
	Method = InArgs._Method;
}


void SMenuAnchor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	SPanel::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	TSharedPtr<SWindow> PopupWindow = PopupWindowPtr.Pin();
	if ( PopupWindow.IsValid() && Method == CreateNewWindow )
	{
		// Figure out where our attached pop-up window should be placed.				
		const FVector2D PopupContentDesiredSize = PopupWindow->GetContent()->GetDesiredSize();
		FGeometry PopupGeometry = ComputeMenuPlacement( AllottedGeometry, PopupContentDesiredSize, Placement.Get( ) );
		const FVector2D NewPosition = PopupGeometry.AbsolutePosition;
		const FVector2D NewSize = PopupGeometry.GetDrawSize( );

		const FSlateRect NewShape = FSlateRect( NewPosition.X, NewPosition.Y, NewPosition.X + NewSize.X, NewPosition.Y + NewSize.Y );

		// We made a window for showing the popup.
		// Update the window's position!
		if( PopupWindow->IsMorphing() )
		{
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
			const FVector2D WindowPosition = PopupWindow->GetPositionInScreen();
			const FVector2D WindowSize = PopupWindow->GetSizeInScreen();
			if ( NewPosition != WindowPosition || NewSize != WindowSize )
			{
				PopupWindow->ReshapeWindow( NewShape );
			}
		}
	}

	/** The tick is ending, so the window was not dismissed this tick. */
	bDismissedThisTick = false;
}

void SMenuAnchor::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	ArrangeSingleChild( AllottedGeometry, ArrangedChildren, Children[0], FVector2D::UnitVector);
	if ( IsOpen() )
	{
		// @todo umg na AllottedGeometry here is in Window-Space when computed via OnPaint.
		//           The incorrect geometry causes the popup to fail workspace-edge tests.
		const FGeometry PopupGeometry = ComputeMenuPlacement( AllottedGeometry, Children[1].GetWidget()->GetDesiredSize(), Placement.Get() );
		ArrangedChildren.AddWidget( FArrangedWidget( Children[1].GetWidget(), PopupGeometry ) );
	}
}

FVector2D SMenuAnchor::ComputeDesiredSize() const
{
	return Children[0].GetWidget()->GetDesiredSize();
}

FChildren* SMenuAnchor::GetChildren()
{
	return &Children;
}

int32 SMenuAnchor::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	FArrangedChildren ArrangedChildren( EVisibility::Visible );
	ArrangeChildren( AllottedGeometry, ArrangedChildren );
	
	// There may be zero elements in this array if our child collapsed/hidden
	if ( ArrangedChildren.Num() > 0 )
	{
		FArrangedWidget& TheChild = ArrangedChildren[0];

		const FSlateRect ChildClippingRect = AllottedGeometry.GetClippingRect().IntersectionWith( MyClippingRect );

		LayerId = TheChild.Widget->Paint( Args.WithNewParent( this ), TheChild.Geometry, ChildClippingRect, OutDrawElements, LayerId + 1, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) );

		if ( IsOpen( ) && ArrangedChildren.Num( ) > 1 && !ChildClippingRect.IsEmpty() )
		{
			OutDrawElements.QueueDeferredPainting(
				FSlateWindowElementList::FDeferredPaint( ArrangedChildren[1].Widget, Args, ArrangedChildren[1].Geometry, MyClippingRect, InWidgetStyle, bParentEnabled ) );
		}
	}

	return LayerId;
}

FGeometry SMenuAnchor::ComputeMenuPlacement( const FGeometry& AllottedGeometry, const FVector2D& PopupDesiredSize, EMenuPlacement PlacementMode )
{
	FVector2D PopupSize = PopupDesiredSize * AllottedGeometry.Scale;

	if ( PlacementMode == MenuPlacement_ComboBox || PlacementMode == MenuPlacement_ComboBoxRight )
	{
		PopupSize = FVector2D( FMath::Max( AllottedGeometry.Size.X, PopupDesiredSize.X ), PopupDesiredSize.Y ) * AllottedGeometry.Scale;
	}

	const float AbsoluteX = (PlacementMode == MenuPlacement_ComboBoxRight)
		? AllottedGeometry.AbsolutePosition.X + AllottedGeometry.GetDrawSize().X - PopupSize.X
		: AllottedGeometry.AbsolutePosition.X;
	const float AbsoluteY = AllottedGeometry.AbsolutePosition.Y;
	FSlateRect Anchor(AbsoluteX, AbsoluteY, AbsoluteX + AllottedGeometry.Size.X*AllottedGeometry.Scale, AbsoluteY + AllottedGeometry.Size.Y*AllottedGeometry.Scale);
	EOrientation Orientation = (PlacementMode == MenuPlacement_MenuRight) ? Orient_Horizontal : Orient_Vertical;
	const FVector2D NewPositionDesktopSpace = FSlateApplication::Get().CalculatePopupWindowPosition( Anchor, PopupSize, Orientation );

	return FGeometry( FVector2D::ZeroVector, NewPositionDesktopSpace, PopupSize / AllottedGeometry.Scale, AllottedGeometry.Scale );
}

void SMenuAnchor::RequestClosePopupWindow( const TSharedRef<SWindow>& PopupWindow )
{
	bDismissedThisTick = true;
	if (ensure(Method == CreateNewWindow))
	{
		FSlateApplication::Get().RequestDestroyWindow(PopupWindow);
		
		if (OnMenuOpenChanged.IsBound())
		{
			OnMenuOpenChanged.Execute(false);
		}
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


void SMenuAnchor::SetContent(TSharedRef<SWidget> InContent)
{
	Children[0]
	.Padding(0)
	[
		InContent
	];
}

void SMenuAnchor::SetMenuContent(TSharedRef<SWidget> InMenuContent)
{
	MenuContent = InMenuContent;
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
			if (OnMenuOpenChanged.IsBound())
			{
				OnMenuOpenChanged.Execute(true);
			}
			
			// This can be called at any time so we use the push menu override that explicitly allows us to specify our parent

			// Figure out where the menu anchor is on the screen, so we can set the initial position of our pop-up window
			SlatePrepass();

			// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
			FWidgetPath MyWidgetPath;
			FSlateApplication::Get().GeneratePathToWidgetChecked( AsShared(), MyWidgetPath );
			const FGeometry& MyGeometry = MyWidgetPath.Widgets.Last().Geometry;

			// @todo Slate: This code does not properly propagate the layout scale of the widget we are creating the popup for.
			// The popup instead has a scale of one, but a computed size as if the contents were scaled. This code should ideally
			// wrap the contents with an SFxWidget that applies the necessary layout scale. This is a very rare case right now.
			
			// Figure out how big the content widget is so we can set the window's initial size properly
			TSharedRef< SWidget > MenuContentRef = MenuContentPtr.ToSharedRef();
			MenuContentRef->SlatePrepass();

			// Combo-boxes never size down smaller than the widget that spawned them, but all
			// other pop-up menus are currently auto-sized
			const FVector2D DesiredContentSize = MenuContentRef->GetDesiredSize();  // @todo: This is ignoring any window border size!
			const EMenuPlacement PlacementMode = Placement.Get();

			const FVector2D NewPosition = MyGeometry.AbsolutePosition;
			FVector2D NewWindowSize = DesiredContentSize;
			const FVector2D SummonLocationSize = MyGeometry.Size;

			FPopupTransitionEffect TransitionEffect( FPopupTransitionEffect::None );
			if( PlacementMode == MenuPlacement_ComboBox || PlacementMode == MenuPlacement_ComboBoxRight )
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
				TSharedRef<SWindow> PopupWindow = FSlateApplication::Get().PushMenu( AsShared(), MenuContentRef, NewPosition, TransitionEffect, bFocusMenu, false, NewWindowSize, SummonLocationSize );
				PopupWindow->SetRequestDestroyWindowOverride( FRequestDestroyWindowOverride::CreateSP(this, &SMenuAnchor::RequestClosePopupWindow) );
				PopupWindowPtr = PopupWindow;
			}
			else
			{
				// We are re-using the current window instead of creating a new one.
				// The popup will be presented via an overlay service.
				ensure(Method==UseCurrentWindow);
				Children[1]
				[
					MenuContentRef
				];

				// We want to support dismissing the popup widget when the user clicks outside it.
				FSlateApplication::Get( ).GetPopupSupport( ).RegisterClickNotification( MenuContentRef, FOnClickedOutside::CreateSP( this, &SMenuAnchor::OnClickedOutsidePopup ) );
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
				Children[1].DetachWidget();
			}

			if (OnMenuOpenChanged.IsBound())
			{
				OnMenuOpenChanged.Execute(false);
			}
		}
	}
}

bool SMenuAnchor::IsOpen() const
{
	return PopupWindowPtr.Pin().IsValid() || Children[1].GetWidget() != SNullWidget::NullWidget;
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

SMenuAnchor::~SMenuAnchor()
{
	TSharedPtr<SWindow> PopupWindow = PopupWindowPtr.Pin();
	if (PopupWindow.IsValid())
	{
		// Close the Popup window.
		if (Method == CreateNewWindow)
		{
			// Request that the popup be closed.
			PopupWindow->RequestDestroyWindow();
		}
		
		// We no longer have a popup open, so reset all the tracking state associated.
		PopupWindowPtr.Reset();
	}
}
