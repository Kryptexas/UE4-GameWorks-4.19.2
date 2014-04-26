// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateEnums.h: Declares various enumeration types for Slate.
=============================================================================*/

#pragma once

#include "SlateEnums.generated.h"


namespace EButtonClickMethod
{
	/**
	 * Enumerates different methods that a button click can be triggered. Normally, DownAndUp is appropriate.
	 */
	enum Type
	{
		/**
		 * User must press the button, then release while over the button to trigger the click.
		 * This is the most common type of button.
		 */
		DownAndUp,

		/**
		 * Click will be triggered immediately on mouse down, and mouse will not be captured.
		 */
		MouseDown,

		/**
		 * Click will always be triggered when mouse button is released over the button,
		 * even if the button wasn't pressed down over it.
		 */
		MouseUp,
	};
}


namespace EFocusMoveDirection
{
	/**
	 * Enumerates directions in which the focus can move.
	 */
	enum Type
	{
		/** Move to next widget. */
		Next,

		/** Move focus to previous widget. */
		Previous
	};
}


/**
 * Enumerates horizontal alignment options, i.e. for widget slots.
 */
UENUM()
enum EHorizontalAlignment
{
	/** Fill the entire width. */
	HAlign_Fill UMETA(DisplayName="Fill"),

	/** Left-align. */
	HAlign_Left UMETA(DisplayName="Left"),

	/** Center-align. */
	HAlign_Center UMETA(DisplayName="Center"),

	/** Right-align. */
	HAlign_Right UMETA(DisplayName="Right"),	
};


/**
 * Enumerates vertical alignment options, i.e. for widget slots.
 */
UENUM()
enum EVerticalAlignment
{
	/** Fill the entire height. */
	VAlign_Fill UMETA(DisplayName="Fill"),

	/** Top-align. */
	VAlign_Top UMETA(DisplayName="Top"),

	/** Center-align. */
	VAlign_Center UMETA(DisplayName="Center"),

	/** Bottom-align. */
	VAlign_Bottom UMETA(DisplayName="Bottom"),
};


/**
 * Enumerates possible placements for pop-up menus.
 */
UENUM()
enum EMenuPlacement
{
	/** Place the menu immediately below the anchor */
	MenuPlacement_BelowAnchor UMETA(DisplayName="Below Anchor"),

	/** Place the menu immediately below the anchor and match is width to the anchor's content */
	MenuPlacement_ComboBox UMETA(DisplayName="Combo Box"),

	/** Place the menu to the right of the anchor */
	MenuPlacement_MenuRight UMETA(DisplayName="Menu Right"),

	/** Place the menu immediately above the anchor, not transition effect */
	MenuPlacement_AboveAnchor UMETA(DisplayName="Above Anchor"),
};


/**
 * Enumerates widget orientations.
 */
UENUM()
enum EOrientation
{
	/** Orient horizontally, i.e. left to right. */
	Orient_Horizontal UMETA(DisplayName="Horizontal"),

	/** Orient vertically, i.e. top to bottom. */
	Orient_Vertical UMETA(DisplayName="Vertical"),
};


/**
 * Enumerates scroll directions.
 */
enum EScrollDirection
{
	/** Scroll down. */
	Scroll_Down UMETA(DisplayName="Down"),

	/** Scroll up. */
	Scroll_Up UMETA(DisplayName="Up"),
};
