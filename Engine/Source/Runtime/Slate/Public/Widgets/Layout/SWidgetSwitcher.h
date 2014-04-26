// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SWidgetSwitcher.h: Declares the SWidgetSwitcher class.
=============================================================================*/

#pragma once


/**
 * Implements a widget switcher.
 *
 * A widget switcher is like a tab control, but without tabs. At most one widget is visible at time.
 */
class SLATE_API SWidgetSwitcher
	: public SPanel
{
public:

	SLATE_BEGIN_ARGS(SWidgetSwitcher)
		: _WidgetIndex(0)
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}

		SLATE_SUPPORTS_SLOT(FSimpleSlot)

		/** Holds the index of the initial widget to be displayed (INDEX_NONE = default). */
		SLATE_ATTRIBUTE(int32, WidgetIndex)

	SLATE_END_ARGS()

public:

	/**
	 * Adds a slot to the widget switcher at the specified location.
	 *
	 * @param SlotIndex - The index at which to insert the slot, or INDEX_NONE to append.
	 */
	FSimpleSlot& AddSlot( int32 SlotIndex = INDEX_NONE );

	/**
	 * Constructs the widget.
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Gets the active widget.
	 *
	 * @return Active widget.
	 */
	TSharedPtr<SWidget> GetActiveWidget( ) const;

	/**
	 * Gets the slot index of the currently active widget.
	 *
	 * @return The slot index, or INDEX_NONE if no widget is active.
	 */
	int32 GetActiveWidgetIndex( ) const
	{
		return WidgetIndex.Get();
	}

	/**
	 * Gets the number of widgets that this switcher manages.
	 */
	int32 GetNumWidgets( ) const
	{
		return AllChildren.Num();
	}

	/**
	 * Gets the widget in the specified slot.
	 *
	 * @param SlotIndex - The slot index of the widget to get.
	 *
	 * @return The widget, or NULL if the slot does not exist.
	 */
	TSharedPtr<SWidget> GetWidget( int32 SlotIndex ) const;

	/**
	 * Gets the slot index of the specified widget.
	 *
	 * @param Widget - The widget to get the index for.
	 *
	 * @return The slot index, or INDEX_NONE if the widget does not exist.
	 */
	int32 GetWidgetIndex( TSharedRef<SWidget> Widget ) const;

	/**
	 * Sets the active widget.
	 *
	 * @param Widget - The widget to activate.
	 */
	void SetActiveWidget( TSharedRef<SWidget> Widget )
	{
		SetActiveWidgetIndex(GetWidgetIndex(Widget));
	}

	/**
	 * Activates the widget at the specified index.
	 *
	 * @param Index - The slot index.
	 */
	void SetActiveWidgetIndex( int32 Index );

public:

	/**
	 * Creates a new widget slot.
	 *
	 * @return A new slot.
	 */
	static FSimpleSlot& Slot( )
	{
		return *(new FSimpleSlot());
	}

protected:

	// Begin SCompoundWidget interface

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	
	virtual FVector2D ComputeDesiredSize( ) const OVERRIDE;

	virtual FChildren* GetChildren( ) OVERRIDE;

	// End SCompoundWidget interface

private:

	/** Required to implement GetChildren() in a way that can dynamically return the currently active child. */
	class SLATE_API FOneDynamicChild
		: public FChildren
	{
	public:

		FOneDynamicChild( TPanelChildren<FSimpleSlot>* InAllChildren = NULL, const TAttribute<int32>* InWidgetIndex = NULL )
			: AllChildren( InAllChildren )
			, WidgetIndex( InWidgetIndex )
		{ }
		
		virtual int32 Num() const OVERRIDE { return AllChildren->Num() > 0 ? 1 : 0; }
		
		virtual TSharedRef<SWidget> GetChildAt( int32 Index ) OVERRIDE { check(Index == 0); return (*AllChildren)[WidgetIndex->Get()].Widget; }
		
		virtual TSharedRef<const SWidget> GetChildAt( int32 Index ) const OVERRIDE { check(Index == 0); return (*AllChildren)[WidgetIndex->Get()].Widget; }
		
	private:

		TPanelChildren<FSimpleSlot>* AllChildren;
		const TAttribute<int32>* WidgetIndex;

	} OneDynamicChild;

	/** Holds the desired widget index */
	TAttribute<int32> WidgetIndex;

	// Holds the collection of widgets.
	TPanelChildren<FSimpleSlot> AllChildren;
};
