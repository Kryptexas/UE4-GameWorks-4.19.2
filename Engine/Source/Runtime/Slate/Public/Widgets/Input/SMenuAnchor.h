// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "STextBlock.h"

/** Notification when popup is opened/closed. */
DECLARE_DELEGATE_OneParam(FOnIsOpenChanged, bool)

/**
 * A PopupAnchor summons a Popup relative to its content.
 * Summoning a popup relative to the cursor is accomplished via the application.
 */
class SLATE_API SMenuAnchor : public SPanel
{
public:
	enum EMethod
	{
		/** Creating a new window allows us to place popups outside of the window in which the menu anchor resides. */
		CreateNewWindow,
		/** Applications that intend to run in fullscreen cannot create new windows. */
		UseCurrentWindow
	};
	
public:
	SLATE_BEGIN_ARGS( SMenuAnchor )
		: _Content()
		, _MenuContent( SNew(STextBlock) .Text( NSLOCTEXT("SMenuAnchor", "NoMenuContent", "No Menu Content Assigned; use .MenuContent") ) )
		, _OnGetMenuContent()
		, _Placement( MenuPlacement_BelowAnchor )
		, _Method( CreateNewWindow )
		{}
		
		SLATE_DEFAULT_SLOT( FArguments, Content )
		
		SLATE_ARGUMENT( TSharedPtr<SWidget>, MenuContent )

		SLATE_EVENT( FOnGetContent, OnGetMenuContent )
		
		SLATE_EVENT( FOnIsOpenChanged, OnMenuOpenChanged )

		SLATE_ATTRIBUTE( EMenuPlacement, Placement )

		SLATE_ARGUMENT( EMethod, Method )

	SLATE_END_ARGS()

	virtual ~SMenuAnchor();
	
	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );
	
	SMenuAnchor();

	/** See Content Slot attribute */
	void SetContent(TSharedRef<SWidget> InContent);

	/** See MenuContent attribute */
	void SetMenuContent(TSharedRef<SWidget> InMenuContent);
	
	/**
	 * Open or close the popup
	 *
	 * @param InIsOpen    If true, open the popup. Otherwise close it.
	 * @param bFocusMenu  Shoudl we focus the popup as soon as it opens?
	 */
	void SetIsOpen( bool InIsOpen, const bool bFocusMenu = true );
	
	/** @return true if the popup is open; false otherwise. */
	bool IsOpen() const;

	/** @return true if we should open the menu due to a click. Sometimes we should not, if
	   the same MouseDownEvent that just closed the menu is about to re-open it because it happens to land on the button.*/
	bool ShouldOpenDueToClick() const;

	/** @return The current menu position */
	FVector2D GetMenuPosition() const;

	/** @return Whether this menu has open submenus */
	bool HasOpenSubMenus() const;

protected:
	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
	virtual FVector2D ComputeDesiredSize( ) const override;
	virtual FChildren* GetChildren() override;
	int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const;
	// End of SWidget interface

	/**
	 * Computes the desired position for a popup that is attached to this anchor.
	 *
	 * @param   AllottedGeometry    Our widget's geometry
	 * @param   PopupDesiredSize    How big the popup needs to be for all of its contents to be visible
	 * @param   PlacementMode       Should the popup be below the anchor? Right of it? Some other placement?
	 *
	 * @return Geometry computed for the popup widget.
	 */
	static FGeometry ComputeMenuPlacement( const FGeometry& AllottedGeometry, const FVector2D& PopupDesiredSize, EMenuPlacement PlacementMode );

	/** Invoked when the popup window is being closed by the application */
	void RequestClosePopupWindow( const TSharedRef<SWindow>& PopupWindow );

	/** Invoked when a click occurred outside the widget that is our popup; used for popup auto-dismissal */
	void OnClickedOutsidePopup();

protected:
	/** A pointer to the window presenting this popup. Pointer is null when the popup is not visible */
	TWeakPtr<SWindow> PopupWindowPtr;

	/** Static menu content to use when the delegate used when OnGetMenuContent is not defined. */
	TSharedPtr<SWidget> MenuContent;
	
	/** Callback invoked when the popup is being summoned */
	FOnGetContent OnGetMenuContent;

	/** Callback invoked when the popup is being opened/closed */
	FOnIsOpenChanged OnMenuOpenChanged;

	/** How should the popup be placed relative to the anchor. */
	TAttribute<EMenuPlacement> Placement;

	/** Was the menu just dismissed this tick? */
	bool bDismissedThisTick;

	/** Should we summon a new window for this popup or  */
	EMethod Method;

	TPanelChildren<FSimpleSlot> Children;
};
