// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SWindow.h"
#include "LayoutUtils.h"


namespace SWindowDefs
{
	/** Height of a Slate window title bar, in pixels */
	static const float DefaultTitleBarSize = 24.0f;

	/** Size of the hit result border for the window borders */
	static const FSlateRect HitResultBorderSize(10,10,10,10);

	/** Actual size of the window borders */
	static const FMargin WindowBorderSize(5,5,5,5);

	/** Size of the corner rounding radius.  Used for regular, non-maximized windows only (not tool-tips or decorators.) */
	static const int32 CornerRadius = 6;

	/** Window flash rate. Flashes per second */
	static const float WindowFlashFrequency = 5.5f;

	/** Window flash duration. Seconds*/
	static const float WindowFlashDuration = 1.0f;
}

/**
 * An internal overlay used by Slate to support in-window pop ups and tooltips.
 * The overlay ignores DPI scaling when it does its own arrangement, but otherwise
 * passes all DPI scale values through.
 */
class SPopupLayer : public SPanel
{
public:
	SLATE_BEGIN_ARGS( SPopupLayer )
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}

		SLATE_SUPPORTS_SLOT( FPopupLayerSlot )

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<SWindow>& InWindow )
	{
		OwnerWindow = InWindow;

		const int32 NumSlots = InArgs.Slots.Num();
		for ( int32 SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex )
		{
			Children.Add( InArgs.Slots[SlotIndex] );
		}
	}

	/** Make a new ListPanel::Slot  */
	FPopupLayerSlot& Slot()
	{
		return *(new FPopupLayerSlot());
	}
	
	/** Add a slot to the ListPanel */
	FPopupLayerSlot& AddSlot(int32 InsertAtIndex = INDEX_NONE)
	{
		FPopupLayerSlot& NewSlot = SPopupLayer::Slot();
		if (InsertAtIndex == INDEX_NONE)
		{
			this->Children.Add( &NewSlot );
		}
		else
		{
			this->Children.Insert( &NewSlot, InsertAtIndex );
		}
	
		return NewSlot;
	}

	void RemoveSlot(const TSharedRef<SWidget>& WidgetToRemove)
	{
		for( int32 CurSlotIndex = 0; CurSlotIndex < Children.Num(); ++CurSlotIndex )
		{
			const FPopupLayerSlot& CurSlot = Children[ CurSlotIndex ];
			if( CurSlot.Widget == WidgetToRemove )
			{
				Children.RemoveAt( CurSlotIndex );
				return;
			}
		}
	}

private:

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE
	{
		const FVector2D WindowDesktopPosition = (ensure(OwnerWindow.IsValid()))
			? OwnerWindow.Pin()->GetPositionInScreen()
			: FVector2D::ZeroVector;
		
		for ( int32 ChildIndex = 0; ChildIndex < Children.Num(); ++ChildIndex )
		{
			const FPopupLayerSlot& CurChild = Children[ChildIndex];
			const EVisibility ChildVisibility = CurChild.Widget->GetVisibility();
			if ( ArrangedChildren.Accepts(ChildVisibility) )
			{
				const FVector2D ChildLocalPosition = CurChild.DesktopPosition_Attribute.Get() - WindowDesktopPosition;
				const FVector2D WidgetDesiredSize = CurChild.Widget->GetDesiredSize();
				const float ChildScale = CurChild.Scale_Attribute.Get();
				// The position is explicitly in desktop pixels.
				// The size and DPI scale come from the widget that is using
				// this overlay to "punch" through the UI.
				ArrangedChildren.AddWidget( ChildVisibility, FArrangedWidget( CurChild.Widget,
					FGeometry(
						ChildLocalPosition / ChildScale,
						AllottedGeometry.AbsolutePosition,
						FVector2D(
							// Notice that override sizes get divided by ChildScale because any explicit size overrides are in pixel units and are not affected by DPI scaling.
							CurChild.WidthOverride_Attribute.IsSet() ? CurChild.WidthOverride_Attribute.Get() / ChildScale : WidgetDesiredSize.X,
							CurChild.HeightOverride_Attribute.IsSet() ? CurChild.HeightOverride_Attribute.Get() / ChildScale : WidgetDesiredSize.Y
						),
						ChildScale
				) ) );
			}
		}
	}

	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		return FVector2D(100,100);
	}

	/**
	 * All widgets must provide a way to access their children in a layout-agnostic way.
	 * Panels store their children in Slots, which creates a dilemma. Most panels
	 * can store their children in a TPanelChildren<Slot>, where the Slot class
	 * provides layout information about the child it stores. In that case
	 * GetChildren should simply return the TPanelChildren<Slot>. See StackPanel for an example.
	 */
	virtual FChildren* GetChildren() OVERRIDE
	{
		return &Children;
	}

	TPanelChildren<FPopupLayerSlot> Children;
	TWeakPtr<SWindow> OwnerWindow;
};

/** Widget that represents the app icon + system menu button, usually drawn in the top left of a Windows app */
class SAppIconWidget : public SCompoundWidget
{
	SLATE_BEGIN_ARGS( SAppIconWidget )
		: _IconColorAndOpacity( FLinearColor::White )
	{}

	/** Icon color and opacity */
	SLATE_ATTRIBUTE( FSlateColor, IconColorAndOpacity )

	SLATE_END_ARGS( )
	
	void Construct( const FArguments& Args )
	{
		this->ChildSlot
			[
				SNew( SBox )
				.Padding( FMargin( 0.0f, 0.0f, 2.0f, 0.0f ) )

				// Don't allow our box or child widgets to be hit tested.  Otherwise GetWindowZoneOverride() won't be called for our widget.
				.Visibility( EVisibility::HitTestInvisible )

				[
					SNew( SImage )
					.Image( FSlateApplication::Get().GetAppIcon() )
					.ColorAndOpacity( Args._IconColorAndOpacity )
				]
			];
	}

	virtual EWindowZone::Type GetWindowZoneOverride() const OVERRIDE
	{
		// Pretend we are a REAL system menu so the user can click to open a menu, or double-click to close the app on Windows
		return EWindowZone::SysMenu;
	}
};


void SWindow::Construct(const FArguments& InArgs)
{
	check(InArgs._Style);
	this->Style = InArgs._Style;
	this->WindowBackground = &InArgs._Style->BackgroundBrush;

	this->Title = InArgs._Title;
	this->bDragAnywhere = InArgs._bDragAnywhere;
	this->bIsTransparent = InArgs._SupportsTransparency;
	this->Opacity = InArgs._InitialOpacity;
	this->bInitiallyMaximized = InArgs._IsInitiallyMaximized;
	this->SizingRule = InArgs._SizingRule;
	this->bIsPopupWindow = InArgs._IsPopupWindow;
	this->bFocusWhenFirstShown = InArgs._FocusWhenFirstShown;
	this->bActivateWhenFirstShown = InArgs._ActivateWhenFirstShown;
	this->bHasOSWindowBorder = InArgs._UseOSWindowBorder;
	this->bHasMinimizeButton = InArgs._SupportsMinimize;
	this->bHasMaximizeButton = InArgs._SupportsMaximize;
	this->bHasSizingFrame = !InArgs._IsPopupWindow && InArgs._SizingRule == ESizingRule::UserSized;
	
	// calculate window size from client size
	const bool bCreateTitleBar = InArgs._CreateTitleBar && !bIsPopupWindow && !bIsCursorDecoratorWindow && !bHasOSWindowBorder;
	FVector2D WindowSize = InArgs._ClientSize;

	// Do not adjust the client size if we have an OS border.  
	if (!HasOSWindowBorder())
	{
		const FMargin BorderSize = GetWindowBorderSize();

		WindowSize.X += BorderSize.Left + BorderSize.Right;
		WindowSize.Y += BorderSize.Bottom + BorderSize.Top;

		if (bCreateTitleBar)
		{
			WindowSize.Y += SWindowDefs::DefaultTitleBarSize;
		}
	}

	// calculate initial window position
	FVector2D WindowPosition = InArgs._ScreenPosition;

	AutoCenterRule = InArgs._AutoCenter;

	// Get desktop metrics
	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics( DisplayMetrics );
	const FPlatformRect& VirtualDisplayRect = DisplayMetrics.VirtualDisplayRect;
	const FPlatformRect& PrimaryDisplayRect = DisplayMetrics.PrimaryDisplayWorkAreaRect;

	// If we're manually positioning the window we need to check if it's outside
	// of the virtual bounds of the current displays or too large.
	if ( AutoCenterRule == EAutoCenter::None && InArgs._SaneWindowPlacement )
	{
		// Check to see if the upper left corner of the window is outside the virtual
		// bounds of the display, if so reset to preferred work area
		if (WindowPosition.X < VirtualDisplayRect.Left ||
			WindowPosition.X >= VirtualDisplayRect.Right ||
			WindowPosition.Y < VirtualDisplayRect.Top ||
			WindowPosition.Y >= VirtualDisplayRect.Bottom)
		{
			AutoCenterRule = EAutoCenter::PreferredWorkArea;
		}

		float PrimaryWidthPadding = DisplayMetrics.PrimaryDisplayWidth - 
			(PrimaryDisplayRect.Right - 1 - PrimaryDisplayRect.Left);
		float PrimaryHeightPadding = DisplayMetrics.PrimaryDisplayHeight - 
			(PrimaryDisplayRect.Bottom - 1 - PrimaryDisplayRect.Top);

		float VirtualWidth = (VirtualDisplayRect.Right - 1 - VirtualDisplayRect.Left);
		float VirtualHeight = (VirtualDisplayRect.Bottom - 1 - VirtualDisplayRect.Top);

		// Make sure that the window size is no larger than the virtual display area.
		WindowSize.X = FMath::Clamp(WindowSize.X, 0.0f, VirtualWidth - PrimaryWidthPadding);
		WindowSize.Y = FMath::Clamp(WindowSize.Y, 0.0f, VirtualHeight - PrimaryHeightPadding);
	}

	if( AutoCenterRule != EAutoCenter::None )
	{
		FSlateRect AutoCenterRect;

		switch( AutoCenterRule )
		{
		default:
		case EAutoCenter::PrimaryWorkArea:
			AutoCenterRect = FSlateRect(
				(float)PrimaryDisplayRect.Left, 
				(float)PrimaryDisplayRect.Top,
				(float)PrimaryDisplayRect.Right,
				(float)PrimaryDisplayRect.Bottom );		
			break;
		case EAutoCenter::PreferredWorkArea:
			AutoCenterRect = FSlateApplication::Get().GetPreferredWorkArea();
			break;
		}

		// Setup a position and size for the main frame window that's centered in the desktop work area
		const FVector2D DisplayTopLeft( AutoCenterRect.Left, AutoCenterRect.Top );
		const FVector2D DisplaySize( AutoCenterRect.Right - AutoCenterRect.Left, AutoCenterRect.Bottom - AutoCenterRect.Top );
		WindowPosition = DisplayTopLeft + ( DisplaySize - WindowSize ) * 0.5f;
	}

	this->InitialDesiredScreenPosition = WindowPosition;
	this->InitialDesiredSize = WindowSize;

	this->ConstructWindowInternals( bCreateTitleBar );
	this->SetContent( InArgs._Content.Widget );
}


TSharedRef<SWindow> SWindow::MakeNotificationWindow()
{
	TSharedRef<SWindow> NewWindow =
		SNew(SWindow)
		.SupportsMaximize( false )
		.SupportsMinimize( false )
		.IsPopupWindow( true )
		.CreateTitleBar( false )
		.SizingRule( ESizingRule::Autosized )
		.SupportsTransparency( true )
		.InitialOpacity( 0.0f )
		.FocusWhenFirstShown( false )
		.ActivateWhenFirstShown( false );

	// Notification windows slide open so we'll mark them as resized frequently
	NewWindow->bSizeWillChangeOften = true;
	NewWindow->ExpectedMaxWidth = 1024;
	NewWindow->ExpectedMaxHeight = 256;

	return NewWindow;
}


TSharedRef<SWindow> SWindow::MakeToolTipWindow()
{
	TSharedRef<SWindow> NewWindow = SNew( SWindow )
		.IsPopupWindow( true )
		.SizingRule( ESizingRule::Autosized )
		.FocusWhenFirstShown( false )
		.ActivateWhenFirstShown( false );
	NewWindow->bIsToolTipWindow = true;
	NewWindow->bIsTopmostWindow = true;
	NewWindow->bIsTransparent = true;
	NewWindow->Opacity = 0.0f;

	// NOTE: These sizes are tweaked for SToolTip widgets (text wrap width of around 400 px)
	NewWindow->bSizeWillChangeOften = true;
	NewWindow->ExpectedMaxWidth = 512;
	NewWindow->ExpectedMaxHeight = 256;

	return NewWindow;
}


TSharedRef<SWindow> SWindow::MakeCursorDecorator()
{
	TSharedRef<SWindow> NewWindow = SNew( SWindow )
		.IsPopupWindow( true )
		.SizingRule( ESizingRule::Autosized )
		.FocusWhenFirstShown( false )
		.ActivateWhenFirstShown( false );
	NewWindow->bIsToolTipWindow = true;
	NewWindow->bIsTopmostWindow = true;
	NewWindow->bIsCursorDecoratorWindow = true;
	NewWindow->bIsTransparent = true;
	NewWindow->Opacity = 1.0f;

	return NewWindow;
}

FVector2D SWindow::ComputeWindowSizeForContent( FVector2D ContentSize )
{
	// @todo mainframe: This code should be updated to handle the case where we're spawning a window that doesn't have 
	//                  a traditional title bar, such as a window that contains a primary SDockingArea.  Currently, the
	//                  size reported here will be too large!
	return ContentSize + FVector2D(0, SWindowDefs::DefaultTitleBarSize) + SWindowDefs::WindowBorderSize.GetDesiredSize();
}


void SWindow::ConstructWindowInternals( const bool bCreateTitleBar )
{
	ForegroundColor = FCoreStyle::Get().GetSlateColor("DefaultForeground");

	// Setup widget that represents the main area of the window.  That is, everything inside the window's border.
	TSharedRef< SVerticalBox > MainWindowArea = 
		SNew( SVerticalBox )
			.Visibility( EVisibility::SelfHitTestInvisible );

	{
		if( bCreateTitleBar )
		{
			// @todo mainframe: Should be measured from actual title bar content widgets.  Don't use a hard-coded size!
			TitleBarSize = SWindowDefs::DefaultTitleBarSize;

			MainWindowArea->AddSlot()
			.AutoHeight()
				[
					SNew(SBorder)
						.Padding(0)
						.Visibility( EVisibility::SelfHitTestInvisible )
						.BorderImage( this, &SWindow::GetWindowTitlebackgroundImage )
					[
						SNew(SOverlay)
						.Visibility( EVisibility::SelfHitTestInvisible )
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Visibility( this, &SWindow::GetWindowFlashVisibility )
							.Image( &Style->FlashTitleBrush )
							.ColorAndOpacity( this, &SWindow::GetWindowTitleAreaColor )
						]
						+SOverlay::Slot()
						[
							MakeTitleBarContent()
						]
					]
				];
		}
		else
		{
			TitleBarSize = 0;
		}

		MainWindowArea->AddSlot()
				.FillHeight(1)
				.Expose(ContentSlot)
			[
				SNew(SSpacer)
			];
	}


	if ( !bIsToolTipWindow && !bIsPopupWindow && !bHasOSWindowBorder )
	{
		// The content of the window should be visible unless we have a full window overlay content
		// in which case the full window overlay content is visible but nothing under it
		TAttribute<EVisibility> WindowContentVisibility( this, &SWindow::GetWindowContentVisibility );
		this->ChildSlot
		[
			SAssignNew(WindowOverlay, SOverlay)
			.Visibility( EVisibility::SelfHitTestInvisible )
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Visibility( WindowContentVisibility )
				.Image( this, &SWindow::GetWindowBackground )
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Visibility( WindowContentVisibility )
				.Image( &Style->BorderBrush )
			]
			+SOverlay::Slot()
			[
				SNew(SBox)
				.Visibility( WindowContentVisibility )
				.Padding(TAttribute<FMargin>( this, &SWindow::GetWindowBorderSize ))
				[
					MainWindowArea
				]
			]
			+ SOverlay::Slot()
			[
				SAssignNew(PopupLayer, SPopupLayer, SharedThis(this))
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Visibility( WindowContentVisibility )
				.Image( this, &SWindow::GetWindowOutline )
				.ColorAndOpacity( this, &SWindow::GetWindowOutlineColor )
			]
		];
	}
	else if( bHasOSWindowBorder )
	{
		this->ChildSlot
		[
			SAssignNew(WindowOverlay, SOverlay)
			+ SOverlay::Slot()
			[
					MainWindowArea
			]
			+ SOverlay::Slot()
			[
				SAssignNew(PopupLayer, SPopupLayer, SharedThis(this))
			]
		];
	}
}

/** Are any of our child windows active? */
bool SWindow::HasActiveChildren() const
{
	for (int32 i = 0; i < ChildWindows.Num(); ++i)
	{
		if ( ChildWindows[i] == FSlateApplication::Get().GetActiveTopLevelWindow() || ChildWindows[i]->HasActiveChildren() )
		{
			return true;
		}
	}

	return false;
}



void SWindow::MakeTitleBarContentWidgets( TSharedPtr< SWidget >& OutLeftContent, TSharedPtr< SWidget >& OutCenterContent, TSharedPtr< SWidget >& OutRightContent )
{
	MinimizeButton = 
		SNew( SButton )
			.IsFocusable(false)
			.IsEnabled( this, &SWindow::HasMinimizeBox )
			.ContentPadding(0)
			.OnClicked( this, &SWindow::MinimizeButton_OnClicked )
			.Cursor( EMouseCursor::Default )
			.ButtonStyle( FCoreStyle::Get(), "NoBorder" )
			[
				SNew(SImage)
				.Image( this, &SWindow::GetMinimizeImage )
				.ColorAndOpacity( this, &SWindow::GetWindowTitleContentColor )
			]
		;

	MaximizeRestoreButton =
		SNew( SButton )
			.IsFocusable(false)
			.IsEnabled( this, &SWindow::HasMaximizeBox )
			.ContentPadding(0)
			.OnClicked( this, &SWindow::MaximizeRestoreButton_OnClicked )
			.Cursor( EMouseCursor::Default )
			.ButtonStyle( FCoreStyle::Get(), "NoBorder" )
			[
				SNew(SImage)
				.Image( this, &SWindow::GetMaximizeRestoreImage )
				.ColorAndOpacity( this, &SWindow::GetWindowTitleContentColor )
			]
		;

	CloseButton =
		SNew( SButton )
			.IsFocusable(false)
			.ContentPadding(0)
			.OnClicked( this, &SWindow::CloseButton_OnClicked )
			.Cursor( EMouseCursor::Default )
			.ButtonStyle( FCoreStyle::Get(), "NoBorder" )
			[
				SNew(SImage)
				.Image( this, &SWindow::GetCloseImage )
				.ColorAndOpacity( this, &SWindow::GetWindowTitleContentColor )
			]
		;

#if PLATFORM_MAC

	// Mac UI layout

	OutLeftContent =
		SNew( SBox )
		.Visibility( EVisibility::SelfHitTestInvisible )
		.Padding( FMargin( 0.0f, 2.0f, 0.0f, 0.0f ) )	// Offset vertically a bit to leave some spacing around the buttons
		[
			SNew( SHorizontalBox )
			.Visibility( EVisibility::SelfHitTestInvisible )
			// Close button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( FMargin( 4.0f, 0.0f, 0.0f, 0.0f ) )
			[
				CloseButton.ToSharedRef()
			]

			// Minimize
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MinimizeButton.ToSharedRef()
			]

			// Maximize/Restore
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MaximizeRestoreButton.ToSharedRef()
			]
		]
	;

	OutCenterContent =
		SNew( SBox )
		.Visibility( EVisibility::SelfHitTestInvisible )
		.Padding( FMargin( 2.0f ) )
		[
			// NOTE: We bind the window's title text to our window's GetTitle method, so that if the
			//       title is changed later, the text will always be visually up to date
			SNew( STextBlock )
			.TextStyle( &Style->TitleTextStyle )
			.Text( this, &SWindow::GetTitle )
		]
	;

	// No right content on Mac
	OutRightContent = SNew( SSpacer );

#else // PLATFORM_MAC

	// Windows UI layout
	OutLeftContent = SNew( SAppIconWidget )
					 .IconColorAndOpacity(this, &SWindow::GetWindowTitleContentColor );

	OutCenterContent =
		SNew( SBox )
		.Visibility( EVisibility::SelfHitTestInvisible )
		.Padding( FMargin( 5.0f, 2.0f, 2.0f, 5.0f ) )
		[
			// NOTE: We bind the window's title text to our window's GetTitle method, so that if the
			//       title is changed later, the text will always be visually up to date
			SNew( STextBlock )
			.Visibility( EVisibility::SelfHitTestInvisible )
			.TextStyle( &Style->TitleTextStyle )
			.Text( this, &SWindow::GetTitle )
		]
	;

	OutRightContent =
		SNew( SBox )
		.Visibility( EVisibility::SelfHitTestInvisible )
		.Padding( FMargin( 2.0f, 0.0f, 0.0f, 0.0f ) )	// Offset vertically a bit to leave some spacing around the buttons
		[
			// Minimize
			SNew( SHorizontalBox )
			.Visibility( EVisibility::SelfHitTestInvisible )
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MinimizeButton.ToSharedRef()
			]

			// Maximize/Restore
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				MaximizeRestoreButton.ToSharedRef()
			]

			// Close button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				CloseButton.ToSharedRef()
			]
		]
	;
#endif // PLATFORM_MAC
}

TSharedRef<SWidget> SWindow::MakeTitleBarContent()
{
	TSharedPtr< SWidget > LeftContent;
	TSharedPtr< SWidget > CenterContent;
	TSharedPtr< SWidget > RightContent;
	MakeTitleBarContentWidgets( LeftContent, CenterContent, RightContent );

	LeftContent->SlatePrepass();
	RightContent->SlatePrepass();
	FVector2D LeftSize = LeftContent->GetDesiredSize();
	FVector2D RightSize = RightContent->GetDesiredSize();
	FVector2D SpacerSize = FVector2D(FMath::Max(LeftSize.X, RightSize.X), FMath::Max(LeftSize.Y, RightSize.Y));

	return
		SAssignNew(TitleArea, SBox)
		.Visibility( EVisibility::SelfHitTestInvisible )
		.HeightOverride( this, &SWindow::GetTitleBarSize )
		[
			SNew(SOverlay)
			.Visibility( EVisibility::SelfHitTestInvisible )
			+ SOverlay::Slot()
			[
				SNew( SHorizontalBox )
				.Visibility( EVisibility::SelfHitTestInvisible )
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				[
					SNew(SSpacer)
					.Size(SpacerSize)
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				.FillWidth(1)
				[
					CenterContent.ToSharedRef()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(SSpacer)
					.Size(SpacerSize)
				]
			]
			+ SOverlay::Slot()
			[
				SNew( SHorizontalBox )
				.Visibility( EVisibility::SelfHitTestInvisible )
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				[
					LeftContent.ToSharedRef()
				]

				+ SHorizontalBox::Slot()
				.FillWidth(1)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					RightContent.ToSharedRef()
				]
			]
		];
}

void SWindow::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if( Morpher.bIsActive && bHasEverBeenDrawn )
	{
		if(Morpher.bIsPendingPlay)
		{
			Morpher.Sequence.Play();
			Morpher.bIsPendingPlay = false;
		}
		if ( Morpher.Sequence.IsPlaying() )
		{
			const float InterpAlpha = Morpher.Sequence.GetLerp();

			if( Morpher.bIsAnimatingWindowSize )
			{
				FSlateRect WindowRect = FMath::Lerp( Morpher.StartingMorphShape, Morpher.TargetMorphShape, InterpAlpha );
				if( WindowRect != GetRectInScreen() )
				{
					check( SizingRule != ESizingRule::Autosized );
					this->ReshapeWindow( WindowRect );
				}
			}
			else
			{
				const FVector2D StartPosition( Morpher.StartingMorphShape.Left, Morpher.StartingMorphShape.Top );
				const FVector2D TargetPosition( Morpher.TargetMorphShape.Left, Morpher.TargetMorphShape.Top );
				const FVector2D NewPosition( FMath::Lerp( StartPosition, TargetPosition, InterpAlpha ) );
				if( NewPosition != this->GetPositionInScreen() )
				{
					this->MoveWindowTo( NewPosition );
				}
			}

			const float NewOpacity = FMath::Lerp( Morpher.StartingOpacity, Morpher.TargetOpacity, InterpAlpha );
			this->SetOpacity( NewOpacity );
		}
		else
		{
			if( Morpher.bIsAnimatingWindowSize )
			{
				if( Morpher.TargetMorphShape != GetRectInScreen() )
				{
					check( SizingRule != ESizingRule::Autosized );
					this->ReshapeWindow( Morpher.TargetMorphShape );
				}
			}
			else
			{
				const FVector2D TargetPosition( Morpher.TargetMorphShape.Left, Morpher.TargetMorphShape.Top );
				if( TargetPosition != this->GetPositionInScreen() )
				{
					this->MoveWindowTo( TargetPosition );
				}
			}

			this->SetOpacity( Morpher.TargetOpacity );
			Morpher.bIsActive = false;
		}
	}
}

FVector2D SWindow::GetInitialDesiredSizeInScreen() const
{
	return InitialDesiredSize;
}

FVector2D SWindow::GetInitialDesiredPositionInScreen() const
{
	return InitialDesiredScreenPosition;
}

FGeometry SWindow::GetWindowGeometryInScreen() const
{
	const float AppScale = FSlateApplication::Get().GetApplicationScale();
	return FGeometry( ScreenPosition/AppScale, FVector2D::ZeroVector, Size/AppScale, AppScale );
}

FGeometry SWindow::GetWindowGeometryInWindow() const
{
	const float AppScale = FSlateApplication::Get().GetApplicationScale();
	return FGeometry( FVector2D::ZeroVector, FVector2D::ZeroVector, Size/AppScale, AppScale );
}

FVector2D SWindow::GetPositionInScreen() const
{
	return ScreenPosition;
}

FVector2D SWindow::GetSizeInScreen() const
{
	return Size;
}

FSlateRect SWindow::GetNonMaximizedRectInScreen() const
{
	int X = 0;
	int Y = 0;
	int Width = 0;
	int Height = 0;
	
	if ( NativeWindow->GetRestoredDimensions(X, Y, Width, Height) )
	{
		return FSlateRect( X, Y, X+Width, Y+Height );
	}
	else
	{
		return GetRectInScreen();
	}
}

FSlateRect SWindow::GetRectInScreen() const
{ 
	return FSlateRect( ScreenPosition.X, ScreenPosition.Y, ScreenPosition.X + Size.X, ScreenPosition.Y + Size.Y );
}

FVector2D SWindow::GetClientSizeInScreen() const
{
	if (HasOSWindowBorder())
	{
		return Size;
	}

	FVector2D ClientSize = Size;
	FMargin BorderSize = GetWindowBorderSize();

	ClientSize.X -= (BorderSize.Left + BorderSize.Right);
	ClientSize.Y -= (BorderSize.Top + BorderSize.Bottom + TitleBarSize);

	return ClientSize;
}

FSlateRect SWindow::GetClippingRectangleInWindow() const
{
	return FSlateRect( 0, 0, Size.X, Size.Y );
}


FMargin SWindow::GetWindowBorderSize() const
{
// Mac didn't want a window border, and consoles don't either, so only do this in Windows
#if PLATFORM_WINDOWS
	if (NativeWindow.IsValid() && NativeWindow->IsMaximized())
	{
		int32 OSWindowBorderSize = NativeWindow->GetWindowBorderSize();
		return FMargin(OSWindowBorderSize);
	}

	return SWindowDefs::WindowBorderSize;
#else
	return FMargin();
#endif
}


void SWindow::MoveWindowTo( FVector2D NewPosition )
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->MoveWindowTo( FMath::Trunc(NewPosition.X), FMath::Trunc(NewPosition.Y) );
	}
	else
	{
		InitialDesiredScreenPosition = NewPosition;
	}
}

void SWindow::ReshapeWindow( FVector2D NewPosition, FVector2D NewSize )
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->ReshapeWindow( FMath::Trunc(NewPosition.X), FMath::Trunc(NewPosition.Y), FMath::Trunc(NewSize.X), FMath::Trunc(NewSize.Y) );
	}
	else
	{
		InitialDesiredScreenPosition = NewPosition;
		InitialDesiredSize = NewSize;
	}

	SetCachedSize( NewSize );
}

void SWindow::ReshapeWindow( const FSlateRect& InNewShape )
{
	ReshapeWindow( FVector2D(InNewShape.Left, InNewShape.Top), FVector2D( InNewShape.Right - InNewShape.Left,  InNewShape.Bottom - InNewShape.Top) );
}

void SWindow::Resize( FVector2D NewSize )
{
	Morpher.Sequence.JumpToEnd();
	if ( Size != NewSize )
	{
		if (NativeWindow.IsValid())
		{
			NativeWindow->ReshapeWindow( FMath::Trunc(ScreenPosition.X), FMath::Trunc(ScreenPosition.Y), FMath::Trunc(NewSize.X), FMath::Trunc(NewSize.Y) );
		}
		else
		{
			InitialDesiredSize = NewSize;
		}
	}
}

FSlateRect SWindow::GetFullScreenInfo() const
{
	if (NativeWindow.IsValid())
	{
		int32 X;
		int32 Y;
		int32 Width;
		int32 Height;

		if ( NativeWindow->GetFullScreenInfo( X, Y, Width, Height ) )
		{
			return FSlateRect( X, Y, X + Width, Y + Height );
		}
	}

	return FSlateRect();
}

void SWindow::SetCachedScreenPosition(FVector2D NewPosition)
{
	ScreenPosition = NewPosition;
	OnWindowMoved.ExecuteIfBound( SharedThis( this ) );
}

void SWindow::SetCachedSize( FVector2D NewSize )
{
	if( NativeWindow.IsValid() )
	{
		NativeWindow->AdjustCachedSize( NewSize );
	}
	Size = NewSize;
}

bool SWindow::IsMorphing() const
{
	return Morpher.bIsActive && Morpher.Sequence.IsPlaying();
}

bool SWindow::IsMorphingSize() const
{
	return IsMorphing() && Morpher.bIsAnimatingWindowSize;
}


void SWindow::MorphToPosition( const FCurveSequence& Sequence, const float TargetOpacity, const FVector2D& TargetPosition )
{
	Morpher.bIsAnimatingWindowSize = false;
	Morpher.Sequence = Sequence;
	Morpher.TargetOpacity = TargetOpacity;
	UpdateMorphTargetPosition( TargetPosition );
	StartMorph();
}


void SWindow::MorphToShape( const FCurveSequence& Sequence, const float TargetOpacity, const FSlateRect& TargetShape )
{
	Morpher.bIsAnimatingWindowSize = true;
	Morpher.Sequence = Sequence;
	Morpher.TargetOpacity = TargetOpacity;
	UpdateMorphTargetShape(TargetShape);
	StartMorph();
}

void SWindow::StartMorph()
{
	Morpher.StartingOpacity = GetOpacity();
	Morpher.StartingMorphShape = FSlateRect( this->ScreenPosition.X, this->ScreenPosition.Y, this->ScreenPosition.X + this->Size.X, this->ScreenPosition.Y + this->Size.Y );
	Morpher.bIsPendingPlay = true;
	Morpher.bIsActive = true;
	Morpher.Sequence.JumpToStart();
}

const FSlateBrush* SWindow::GetWindowBackground() const
{
	return WindowBackground;
}

const FSlateBrush* SWindow::GetWindowOutline() const
{
	return &Style->OutlineBrush;
}

FSlateColor SWindow::GetWindowOutlineColor() const
{
	return Style->OutlineColor;
}

void SWindow::UpdateMorphTargetShape( const FSlateRect& TargetShape )
{
	Morpher.TargetMorphShape = TargetShape;
}

void SWindow::UpdateMorphTargetPosition( const FVector2D& TargetPosition )
{
	Morpher.TargetMorphShape.Left = Morpher.TargetMorphShape.Right = TargetPosition.X;
	Morpher.TargetMorphShape.Top = Morpher.TargetMorphShape.Bottom = TargetPosition.Y;
}

FVector2D SWindow::GetMorphTargetPosition() const
{
	return FVector2D( Morpher.TargetMorphShape.Left, Morpher.TargetMorphShape.Top );
}


FSlateRect SWindow::GetMorphTargetShape() const
{
	return Morpher.TargetMorphShape;
}

void SWindow::FlashWindow()
{
	TitleFlashSequence = FCurveSequence(0, SWindowDefs::WindowFlashDuration, ECurveEaseFunction::Linear);
	TitleFlashSequence.Play();
}

void SWindow::BringToFront( bool bForce )
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->BringToFront( bForce );
	}
}

void SWindow::HACK_ForceToFront()
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->HACK_ForceToFront();
	}
}

TSharedPtr<FGenericWindow> SWindow::GetNativeWindow()
{
	return NativeWindow;
}

TSharedPtr<const FGenericWindow> SWindow::GetNativeWindow() const
{
	return NativeWindow;
} 

bool SWindow::IsDescendantOf( const TSharedPtr<SWindow>& ParentWindow ) const
{
	TSharedPtr<SWindow> CandidateToCheck = this->GetParentWindow();
	
	// Keep checking our parent until we get to the root of the tree or find the window we were looking for.
	while (CandidateToCheck.IsValid())
	{
		if (CandidateToCheck == ParentWindow)
		{
			// One of our ancestor windows is the ParentWindow we were looking for!
			return true;
		}

		// Consider the next ancestor
		CandidateToCheck = CandidateToCheck->GetParentWindow();
	}

	return false;
}

void SWindow::SetNativeWindow( TSharedRef<FGenericWindow> InNativeWindow )
{
	check( ! NativeWindow.IsValid() );
	NativeWindow = InNativeWindow;
}

void SWindow::SetContent( TSharedRef<SWidget> InContent )
{
	if ( bIsPopupWindow || bIsCursorDecoratorWindow )
	{
		this->ChildSlot.operator[]( InContent );
	}
	else
	{
		this->ContentSlot->operator[]( InContent );
	}
}

TSharedRef<const SWidget> SWindow::GetContent() const
{
	if ( bIsPopupWindow || bIsCursorDecoratorWindow )
	{
		return this->ChildSlot.GetChildAt(0);
	}
	else
	{
		return this->ContentSlot->Widget;
	}
}

SOverlay::FOverlaySlot& SWindow::AddOverlaySlot( const int32 ZOrder )
{
	if(!WindowOverlay.IsValid())
	{
		ensureMsg( false, TEXT("This window does not support overlays. The added slot will not be visible!") );
		WindowOverlay = SNew(SOverlay).Visibility( EVisibility::HitTestInvisible );
	}

	return WindowOverlay->AddSlot(ZOrder);
}

void SWindow::RemoveOverlaySlot( const TSharedRef<SWidget>& InContent )
{
	if(WindowOverlay.IsValid())
	{
		WindowOverlay->RemoveSlot( InContent );
	}
}

/** Return a new slot in the popup layer. Assumes that the window has a popup layer. */
FPopupLayerSlot& SWindow::AddPopupLayerSlot()
{
	ensure( PopupLayer.IsValid() );
	return PopupLayer->AddSlot();
}

/** Counterpart to AddPopupLayerSlot */
void SWindow::RemovePopupLayerSlot( const TSharedRef<SWidget>& WidgetToRemove )
{
	PopupLayer->RemoveSlot( WidgetToRemove );
}

/** @return should this window show up in the taskbar */
bool SWindow::AppearsInTaskbar() const
{
	return !bIsPopupWindow && !bIsToolTipWindow && !bIsCursorDecoratorWindow;
}

void SWindow::SetOnWindowDeactivated( const FOnWindowDeactivated& InDelegate )
{
	OnWindowDeactivated = InDelegate;
}

/** Sets the delegate to execute right before the window is closed */
void SWindow::SetOnWindowClosed( const FOnWindowClosed& InDelegate )
{
	OnWindowClosed = InDelegate;
}

/** Sets the delegate to execute right after the window has been moved */
void SWindow::SetOnWindowMoved( const FOnWindowMoved& InDelegate)
{
	OnWindowMoved = InDelegate;
}

/** Sets the delegate to override RequestDestroyWindow */
void SWindow::SetRequestDestroyWindowOverride( const FRequestDestroyWindowOverride& InDelegate )
{
	RequestDestroyWindowOverride = InDelegate;
}

/** Request that this window be destroyed. The window is not destroyed immediately. Instead it is placed in a queue for destruction on next Tick */
void SWindow::RequestDestroyWindow()
{
	if( RequestDestroyWindowOverride.IsBound() )
	{
		RequestDestroyWindowOverride.Execute( SharedThis(this) );
	}
	else
	{
		FSlateApplication::Get().RequestDestroyWindow( SharedThis(this) );
	}
}

/** Warning: use Request Destroy Window whenever possible!  This method destroys the window immediately! */
void SWindow::DestroyWindowImmediately()
{
	check( NativeWindow.IsValid() );

	// Destroy the native window
	NativeWindow->Destroy();
}

/** Calls the OnWindowClosed delegate when this window is about to be closed */
void SWindow::NotifyWindowBeingDestroyed()
{
	OnWindowClosed.ExecuteIfBound( SharedThis( this ) );
}

/** Make the window visible */
void SWindow::ShowWindow()
{
	// Make sure the viewport is setup for this window
	if( !bHasEverBeenShown )
	{
		if( ensure( NativeWindow.IsValid() ) )
		{
			// We can only create a viewport after the window has been shown (otherwise the swap chain creation may fail)
			FSlateApplication::Get().GetRenderer()->CreateViewport( SharedThis( this ) );
		}

		// Auto sized windows don't know their size until after their position is set.
		// Repositioning the window on show with the new size solves this.
		if ( SizingRule == ESizingRule::Autosized && AutoCenterRule != EAutoCenter::None )
		{
			SlatePrepass();
			ReshapeWindow( InitialDesiredScreenPosition - (GetDesiredSize() * 0.5f), GetDesiredSize() );
		}

		// Set the window to be maximized if we need to.  Note that this won't actually show the window if its not
		// already shown.
		InitialMaximize();
	}

	bHasEverBeenShown = true;

	if (NativeWindow.IsValid())
	{
		NativeWindow->Show();

		// If this is a tompost window (like a tooltip), make sure that its always rendered top most
		if( IsTopmostWindow() )
		{
			NativeWindow->BringToFront();
		}
	}
}

/** Make the window invisible */
void SWindow::HideWindow()
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->Hide();
	}
}

void SWindow::EnableWindow( bool bEnable )
{
	NativeWindow->Enable( bEnable );

	for( int32 ChildIndex = 0; ChildIndex < ChildWindows.Num(); ++ChildIndex )
	{
		ChildWindows[ChildIndex]->EnableWindow( bEnable );
	}
}


/** @return true if the window is visible, false otherwise*/
bool SWindow::IsVisible() const
{
	return NativeWindow.IsValid() && NativeWindow->IsVisible();
}

bool SWindow::IsWindowMaximized() const
{
	return NativeWindow->IsMaximized();
}

/** Maximize the window if bInitiallyMaximized is set */
void SWindow::InitialMaximize()
{
	if (NativeWindow.IsValid() && bInitiallyMaximized)
	{
		NativeWindow->Maximize();
	}
}

/**
 * Sets the opacity of this window
 *
 * @param	InOpacity	The new window opacity represented as a floating point scalar
 */
void SWindow::SetOpacity( const float InOpacity )
{
	if( Opacity != InOpacity )
	{
		check( NativeWindow.IsValid() );
		Opacity = InOpacity;
		NativeWindow->SetOpacity( Opacity );
	}
}


/** @return the window's current opacity */
float SWindow::GetOpacity() const
{
	return Opacity;
}

bool SWindow::SupportsTransparency() const
{
	return bIsTransparent;
}


/** @return A String representation of the widget */
FString SWindow::ToString() const
{
	return FString::Printf( *NSLOCTEXT("SWindow", "Window_Title", " Window : %s ").ToString(), *GetTitle().ToString() );
}

/** @return true if the window should be activated when first shown */
bool SWindow::ActivateWhenFirstShown() const
{
	return bActivateWhenFirstShown;
}

/** @return true if the window accepts input; false if the window is non-interactive */
bool SWindow::AcceptsInput() const
{
	return !bIsCursorDecoratorWindow && !bIsToolTipWindow;
}

/** @return true if the user decides the size of the window; false if the content determines the size of the window */
bool SWindow::IsUserSized() const
{
	return SizingRule == ESizingRule::UserSized;
}

bool SWindow::IsAutosized() const
{
	return SizingRule == ESizingRule::Autosized;
}

void SWindow::SetSizingRule( ESizingRule::Type InSizingRule )
{
	SizingRule = InSizingRule;
}

/** @return true if this is a vanilla window, or one being used for some special purpose: e.g. tooltip or menu */
bool SWindow::IsRegularWindow() const
{
	return !bIsPopupWindow && !bIsToolTipWindow && !bIsCursorDecoratorWindow;
}

/** @return true if the window should be on top of all other windows; false otherwise */
bool SWindow::IsTopmostWindow() const
{
	return bIsTopmostWindow;
}

/** @return true if mouse coordinates is within this window */
bool SWindow::IsScreenspaceMouseWithin(FVector2D ScreenspaceMouseCoordinate) const
{
	const FVector2D LocalMouseCoordinate = GetWindowGeometryInScreen().AbsoluteToLocal(ScreenspaceMouseCoordinate);
	return NativeWindow->IsPointInWindow(FMath::Trunc(LocalMouseCoordinate.X), FMath::Trunc(LocalMouseCoordinate.Y));
}

/** @return true if this is a user-sized window with a thick edge */
bool SWindow::HasSizingFrame() const
{
	return bHasSizingFrame;
}

/** @return true if this window has a maximize button/box on the titlebar area */
bool SWindow::HasMaximizeBox() const
{
	return bHasMaximizeButton;
}

/** @return true if this window has a minimize button/box on the titlebar area */
bool SWindow::HasMinimizeBox() const
{
	return bHasMinimizeButton;
}

FCursorReply SWindow::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
#if !PLATFORM_MAC // On Mac we depend on system's window resizing
	if (bHasSizingFrame)
	{
		if (WindowZone == EWindowZone::TopLeftBorder || WindowZone == EWindowZone::BottomRightBorder)
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);
		}
		else if (WindowZone == EWindowZone::BottomLeftBorder || WindowZone == EWindowZone::TopRightBorder)
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeSouthWest);
		}
		else if (WindowZone == EWindowZone::TopBorder || WindowZone == EWindowZone::BottomBorder)
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeUpDown);
		}
		else if (WindowZone == EWindowZone::LeftBorder || WindowZone == EWindowZone::RightBorder)
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
		}
	}
#endif
	return FCursorReply::Unhandled();
}

bool SWindow::OnIsActiveChanged( const FWindowActivateEvent& ActivateEvent )
{
	const bool bWasDeactivated = ( ActivateEvent.GetActivationType() == FWindowActivateEvent::EA_Deactivate );
	if ( bWasDeactivated )
	{
		OnWindowDeactivated.ExecuteIfBound();

		const EWindowMode::Type WindowMode = GetWindowMode();
		// If the window is not fullscreen, we do not want to automatically recapture the mouse unless an external UI such as Steam is open. Fullscreen windows we do.
		if( WindowMode != EWindowMode::Fullscreen && WidgetToFocusOnActivate.IsValid() && WidgetToFocusOnActivate.Pin()->HasMouseCapture() && !FSlateApplication::Get().IsExternalUIOpened())
		{
			//For a windowed application with an OS border, if the user is giving focus back to the application by clicking on the close/(X) button, then we must clear 
			//the weak pointer to WidgetToFocus--so that the application's main viewport does not steal focus immediately (thus canceling the close attempt).
			
			//This change introduces a different bug where slate context is lost when closing popup menus.  However, this issue is negated by a 
			//change to FMenuStack::PushMenu, where we ReleaseMouseCapture when immediately shifting focus.
			WidgetToFocusOnActivate.Reset();
		}
	}

	return true;
}

FReply SWindow::MinimizeButton_OnClicked()
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->Minimize();
	}
	return FReply::Handled();
}

FReply SWindow::MaximizeRestoreButton_OnClicked()
{
	if (NativeWindow.IsValid())
	{
		if (NativeWindow->IsMaximized())
		{
			NativeWindow->Restore();
		}
		else
		{
			NativeWindow->Maximize();
		}
	}

	return FReply::Handled();
}

FReply SWindow::CloseButton_OnClicked()
{
	RequestDestroyWindow();
	return FReply::Handled();
}


const FSlateBrush* SWindow::GetMinimizeImage() const
{
	if ( !bHasMinimizeButton )
	{
		return &Style->MinimizeButtonStyle.Disabled;
	}
	else if ( this->MinimizeButton->IsPressed() )
	{
		return &Style->MinimizeButtonStyle.Pressed;
	}
#if PLATFORM_MAC
	else if ( this->MinimizeButton->IsHovered() || MaximizeRestoreButton->IsHovered() || CloseButton->IsHovered() )
#else
	else if ( this->MinimizeButton->IsHovered() )
#endif
	{
		return &Style->MinimizeButtonStyle.Hovered;
	}
#if PLATFORM_MAC
	else if ( NativeWindow.IsValid() && !NativeWindow->IsForegroundWindow() )
	{
		return &Style->MinimizeButtonStyle.Disabled;
	}
#endif
	else
	{
		return &Style->MinimizeButtonStyle.Normal;
	}
}

const FSlateBrush* SWindow::GetMaximizeRestoreImage() const
{
	if (NativeWindow.IsValid() && NativeWindow->IsMaximized())
	{
		if ( !bHasMaximizeButton )
		{
			return &Style->MaximizeButtonStyle.Disabled;
		}
		else if (MaximizeRestoreButton->IsPressed())
		{
			return &Style->RestoreButtonStyle.Pressed;
		}
#if PLATFORM_MAC
		else if ( this->MinimizeButton->IsHovered() || MaximizeRestoreButton->IsHovered() || CloseButton->IsHovered() )
#else
		else if ( MaximizeRestoreButton->IsHovered() )
#endif
		{
			return &Style->RestoreButtonStyle.Hovered;
		}
#if PLATFORM_MAC
		else if ( NativeWindow.IsValid() && !NativeWindow->IsForegroundWindow() )
		{
			return &Style->RestoreButtonStyle.Disabled;
		}
#endif
		else
		{
			return &Style->RestoreButtonStyle.Normal;
		}
	}
	else
	{
		if ( !bHasMaximizeButton )
		{
			return &Style->MaximizeButtonStyle.Disabled;
		}
		else if (MaximizeRestoreButton->IsPressed())
		{
			return &Style->MaximizeButtonStyle.Pressed;
		}
#if PLATFORM_MAC
		else if ( this->MinimizeButton->IsHovered() || MaximizeRestoreButton->IsHovered() || CloseButton->IsHovered() )
#else
		else if ( MaximizeRestoreButton->IsHovered() )
#endif
		{
			return &Style->MaximizeButtonStyle.Hovered;
		}
#if PLATFORM_MAC
		else if ( NativeWindow.IsValid() && !NativeWindow->IsForegroundWindow() )
		{
			return &Style->MaximizeButtonStyle.Disabled;
		}
#endif
		else
		{
			return &Style->MaximizeButtonStyle.Normal;
		}
	}
}

const FSlateBrush* SWindow::GetCloseImage() const
{
	if (CloseButton->IsPressed())
	{
		return &Style->CloseButtonStyle.Pressed;
	}
#if PLATFORM_MAC
	else if ( this->MinimizeButton->IsHovered() || MaximizeRestoreButton->IsHovered() || CloseButton->IsHovered() )
#else
	else if ( CloseButton->IsHovered() )
#endif
	{
		return &Style->CloseButtonStyle.Hovered;
	}
#if PLATFORM_MAC
	else if ( NativeWindow.IsValid() && !NativeWindow->IsForegroundWindow() )
	{
		return &Style->CloseButtonStyle.Disabled;
	}
#endif
	else
	{
		return &Style->CloseButtonStyle.Normal;
	}
}



void SWindow::Maximize()
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->Maximize();
	}
}

void SWindow::Restore()
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->Restore();
	}
}

void SWindow::Minimize()
{
	if (NativeWindow.IsValid())
	{
		NativeWindow->Minimize();
	}
}

int32 SWindow::GetCornerRadius()
{
	return IsRegularWindow() ? SWindowDefs::CornerRadius : 0;
}

bool SWindow::SupportsKeyboardFocus() const
{
	return !bIsToolTipWindow && !bIsCursorDecoratorWindow;
}

FReply SWindow::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	// If we're becoming active and we were set to restore keyboard focus to a specific widget
	// after reactivating, then do so now
	TSharedPtr< SWidget > PinnedWidgetToFocusOnActivate( WidgetToFocusOnActivate.Pin() );

	if( PinnedWidgetToFocusOnActivate.IsValid() && 
		( InKeyboardFocusEvent.GetCause() == EKeyboardFocusCause::WindowActivate  || FSlateApplication::Get().IsExternalUIOpened() ) )
	{
		TArray< TSharedRef<SWindow> > JustThisWindow;
		{
			JustThisWindow.Add( SharedThis(this) );
		}

		FWidgetPath WidgetToFocusPath;
		if( FSlateApplication::Get().FindPathToWidget( JustThisWindow, PinnedWidgetToFocusOnActivate.ToSharedRef(), WidgetToFocusPath ) )
		{
			FSlateApplication::Get().SetKeyboardFocus( WidgetToFocusPath, EKeyboardFocusCause::SetDirectly );
		}
	}

	return FReply::Handled();
}

FReply SWindow::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (bDragAnywhere && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SWindow::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (bDragAnywhere &&  this->HasMouseCapture() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().ReleaseMouseCapture();
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply SWindow::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( bDragAnywhere && this->HasMouseCapture() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) )
	{
		this->MoveWindowTo( ScreenPosition + MouseEvent.GetCursorDelta() );
		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}

}

FVector2D SWindow::ComputeDesiredSize() const
{
	const float Scale = FSlateApplication::Get().GetApplicationScale();
	return SCompoundWidget::ComputeDesiredSize() * Scale;
}

const TArray< TSharedRef<SWindow> >& SWindow::GetChildWindows() const
{
	return ChildWindows;
}

TArray< TSharedRef<SWindow> >& SWindow::GetChildWindows()
{
	return ChildWindows;
}

void SWindow::AddChildWindow( const TSharedRef<SWindow>& ChildWindow )
{
	TSharedPtr<SWindow> PreviousParent = ChildWindow->ParentWindowPtr.Pin();
	if (PreviousParent.IsValid())
	{
		// This child already had a parent, so we are actually re-parenting it
		const bool bRemovedSuccessfully = PreviousParent->RemoveDescendantWindow(ChildWindow);
		check(bRemovedSuccessfully);
	}

	ChildWindow->ParentWindowPtr = SharedThis(this);
	ChildWindow->WindowBackground = &Style->ChildBackgroundBrush;

	FSlateApplication::ArrangeWindowToFront( ChildWindows, ChildWindow );
}

TSharedPtr<SWindow> SWindow::GetParentWindow() const
{
	return ParentWindowPtr.Pin();
}


TSharedPtr<SWindow> SWindow::GetTopmostAncestor()
{
	TSharedPtr<SWindow> TopmostParentSoFar = SharedThis(this);
	while ( TopmostParentSoFar->ParentWindowPtr.IsValid() )
	{
		TopmostParentSoFar = TopmostParentSoFar->ParentWindowPtr.Pin();
	}

	return TopmostParentSoFar;
}

bool SWindow::RemoveDescendantWindow( const TSharedRef<SWindow>& DescendantToRemove )
{
	const bool bRemoved = 0 != ChildWindows.Remove(DescendantToRemove);

	for ( int32 ChildIndex=0; ChildIndex < ChildWindows.Num(); ++ChildIndex )
	{
		TSharedRef<SWindow>& ChildWindow = ChildWindows[ChildIndex];
		if ( ChildWindow->RemoveDescendantWindow( DescendantToRemove ))
		{
			// Reset to the non-child background style
			ChildWindow->WindowBackground = &Style->BackgroundBrush;
			return true;
		}
	}

	return false;
}

void SWindow::SetOnWorldSwitchHack( FOnSwitchWorldHack& InOnSwitchWorldHack )
{
	OnWorldSwitchHack = InOnSwitchWorldHack;
}

int32 SWindow::SwitchWorlds( int32 WorldId ) const
{
	return OnWorldSwitchHack.IsBound() ? OnWorldSwitchHack.Execute( WorldId ) : false;
}

bool PointWithinSlateRect(const FVector2D& Point, const FSlateRect& Rect)
{
	return Point.X >= Rect.Left && Point.X < Rect.Right &&
		Point.Y >= Rect.Top && Point.Y < Rect.Bottom;
}

EWindowZone::Type SWindow::GetCurrentWindowZone(FVector2D LocalMousePosition)
{
	// Don't allow position/resizing of window while in fullscreen mode by ignoring Title Bar/Border Zones
	if ( GetWindowMode() == EWindowMode::WindowedFullscreen || GetWindowMode() == EWindowMode::Fullscreen )
	{
		return EWindowZone::ClientArea;
	}
	else if(LocalMousePosition.X >= 0 && LocalMousePosition.X < Size.X &&
			LocalMousePosition.Y >= 0 && LocalMousePosition.Y < Size.Y)
	{
		int32 Row = 1;
		int32 Col = 1;
		if (LocalMousePosition.X < SWindowDefs::HitResultBorderSize.Left)
		{
			Col = 0;
		}
		else if (LocalMousePosition.X >= Size.X - SWindowDefs::HitResultBorderSize.Right)
		{
			Col = 2;
		}
		if (LocalMousePosition.Y < SWindowDefs::HitResultBorderSize.Top)
		{
			Row = 0;
		}
		else if (LocalMousePosition.Y >= Size.Y - SWindowDefs::HitResultBorderSize.Bottom)
		{
			Row = 2;
		}

		// The actual border is smaller than the hit result zones
		// This grants larger corner areas to grab onto
		bool bInBorder =	LocalMousePosition.X < SWindowDefs::WindowBorderSize.Left ||
							LocalMousePosition.X >= Size.X - SWindowDefs::WindowBorderSize.Right ||
							LocalMousePosition.Y < SWindowDefs::WindowBorderSize.Top ||
							LocalMousePosition.Y >= Size.Y - SWindowDefs::WindowBorderSize.Bottom;

		if (!bInBorder)
		{
			Row = 1;
			Col = 1;
		}

		static const EWindowZone::Type TypeZones[3][3] = 
		{
			{EWindowZone::TopLeftBorder,		EWindowZone::TopBorder,		EWindowZone::TopRightBorder},
			{EWindowZone::LeftBorder,			EWindowZone::ClientArea,	EWindowZone::RightBorder},
			{EWindowZone::BottomLeftBorder,		EWindowZone::BottomBorder,	EWindowZone::BottomRightBorder},
		};

		EWindowZone::Type InZone = TypeZones[Row][Col];
		if (InZone == EWindowZone::ClientArea)
		{
			// Hittest to see if the widget under the mouse should be treated as a title bar (i.e. should move the window)
			TArray< TSharedRef<SWindow> > ThisWindow;
			{
				ThisWindow.Add( SharedThis(this) );
			}

			FWidgetPath HitTestResults = FSlateApplication::Get().LocateWindowUnderMouse(FSlateApplication::Get().GetCursorPos(), ThisWindow);
			if( HitTestResults.Widgets.Num() > 0 )
			{
				const EWindowZone::Type ZoneOverride = HitTestResults.Widgets.Last().Widget->GetWindowZoneOverride();
				if( ZoneOverride != EWindowZone::Unspecified )
				{
					// The widget overrode the window zone
					InZone = ZoneOverride;
				}
				else if( HitTestResults.Widgets.Last().Widget == AsShared() )
				{
					// The window itself was hit, so check for a traditional title bar
					if( (LocalMousePosition.Y - SWindowDefs::WindowBorderSize.Top) < TitleBarSize )
					{
						InZone = EWindowZone::TitleBar;
					}
				}
			}
		}

		WindowZone = InZone;
	}
	else
	{
		WindowZone = EWindowZone::NotInWindow;
	}
	return WindowZone;
}

/**
 * Default constructor. Protected because SWindows must always be used via TSharedPtr. Instead, use FSlateApplication::MakeWindow()
 */
SWindow::SWindow()
	: Opacity( 1.0f )
	, SizingRule( ESizingRule::UserSized )
	, bIsTransparent( false )
	, bIsPopupWindow( false )
	, bIsToolTipWindow( false )
	, bIsTopmostWindow( false )
	, bSizeWillChangeOften( false )
	, bIsCursorDecoratorWindow( false )
	, bInitiallyMaximized( false )
	, bHasEverBeenShown( false )
	, bHasEverBeenDrawn( false )
	, bFocusWhenFirstShown(true)
	, bActivateWhenFirstShown(true)
	, bHasOSWindowBorder( false )
	, bHasMinimizeButton( false )
	, bHasMaximizeButton( false )
	, bHasSizingFrame( false )
	, InitialDesiredScreenPosition( FVector2D::ZeroVector )
	, InitialDesiredSize( FVector2D::ZeroVector )
	, ScreenPosition( FVector2D::ZeroVector )
	, PreFullscreenPosition( FVector2D::ZeroVector )
	, Size( FVector2D::ZeroVector )
	, TitleBarSize( SWindowDefs::DefaultTitleBarSize )
	, ContentSlot(NULL)
	, Style( &FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window") )
	, WindowBackground( &Style->BackgroundBrush )
	, bShouldShowWindowContentDuringOverlay( false )
	, ExpectedMaxWidth( INDEX_NONE )
	, ExpectedMaxHeight( INDEX_NONE )
{
}


/** @return An appropriate resource for the window title background depending on whether the window is active */
const FSlateBrush* SWindow::GetWindowTitlebackgroundImage() const
{
	const bool bIsActive = FSlateApplication::Get().GetActiveTopLevelWindow() == SharedThis(this) || HasActiveChildren();
	return bIsActive ? &Style->ActiveTitleBrush : &Style->InactiveTitleBrush;
}

float SWindow::GetFlashValue() const
{
	if (TitleFlashSequence.IsPlaying())
	{
		float Lerp = TitleFlashSequence.GetLerp();

		const float SinRateMultiplier = 2.0f * PI * SWindowDefs::WindowFlashDuration * SWindowDefs::WindowFlashFrequency;
		float SinTerm = 0.5f * (FMath::Sin( Lerp * SinRateMultiplier ) + 1.0f);

		float FadeTerm = 1.0f - Lerp;

		return SinTerm * FadeTerm;
	}
	return 0.0f;
}

FSlateColor SWindow::GetWindowTitleAreaColor() const
{	
	// Color of the white flash in the title area
	float Flash = GetFlashValue();

	float Alpha = Flash * 0.4f;

	FLinearColor Color = FLinearColor::White;
	Color.A = Alpha;
	return Color;
}

FSlateColor SWindow::GetWindowTitleContentColor() const
{	
	// Color of the title area contents - modulates the icon and buttons
	float Flash = GetFlashValue();

	return FMath::Lerp( FLinearColor::White, FLinearColor::Black, Flash );
}

FOptionalSize SWindow::GetTitleBarSize() const
{
	return TitleBarSize;
}


void SWindow::SetFullWindowOverlayContent( TSharedPtr<SWidget> InContent )
{
	if( FullWindowOverlayWidget.IsValid() )
	{
		// Remove the last slot
		WindowOverlay->RemoveSlot( FullWindowOverlayWidget.ToSharedRef() );
		FullWindowOverlayWidget.Reset();
	}

	if( InContent.IsValid() )
	{
		FullWindowOverlayWidget = InContent;

		// Create a slot in our overlay to hold the content
		WindowOverlay->AddSlot( 1 )
		[
			InContent.ToSharedRef()
		];
	}
}

/** Toggle window between fullscreen and normal mode */
void SWindow::SetWindowMode( EWindowMode::Type NewWindowMode )
{
	EWindowMode::Type CurrentWindowMode = NativeWindow->GetWindowMode();

	if( CurrentWindowMode != NewWindowMode )
	{
		bool bFullscreen = NewWindowMode != EWindowMode::Windowed;

		bool bWasFullscreen = CurrentWindowMode != EWindowMode::Windowed;

		// We need to store off the screen position when entering fullscreen so that we can move the window back to its original position after leaving fullscreen
		if( bFullscreen )
		{
			PreFullscreenPosition = ScreenPosition;
		}

		NativeWindow->SetWindowMode( NewWindowMode );
	
		FSlateApplication::Get().GetRenderer()->UpdateFullscreenState( SharedThis(this), Size.X, Size.Y );


		if( TitleArea.IsValid() )
		{
			// Collapse the Window title bar when switching to Fullscreen
			TitleArea->SetVisibility( (NewWindowMode == EWindowMode::Fullscreen || NewWindowMode == EWindowMode::WindowedFullscreen ) ? EVisibility::Collapsed : EVisibility::Visible );
		}

		if( bWasFullscreen )
		{
			// If we left fullscreen, reset the screen position;
			ScreenPosition = PreFullscreenPosition;
		}
	}

}

bool SWindow::HasFullWindowOverlayContent() const
{
	return FullWindowOverlayWidget.IsValid();
}

void SWindow::BeginFullWindowOverlayTransition()
{
	bShouldShowWindowContentDuringOverlay = true; 
}

void SWindow::EndFullWindowOverlayTransition()
{
	bShouldShowWindowContentDuringOverlay = false;
}

EVisibility SWindow::GetWindowContentVisibility() const
{
	// The window content should be visible if there is no fullscreen overlay or we are showing the content due to a transition to/from a fullscreen overlay
	return (bShouldShowWindowContentDuringOverlay == true || !FullWindowOverlayWidget.IsValid()) ? EVisibility::SelfHitTestInvisible : EVisibility::Hidden;
};

EVisibility SWindow::GetWindowFlashVisibility() const
{
	return TitleFlashSequence.IsPlaying() ? EVisibility::SelfHitTestInvisible : EVisibility::Hidden;
}

