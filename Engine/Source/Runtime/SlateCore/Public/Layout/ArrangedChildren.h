// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ArrangedChildren.h: Declares the FArrangedChildren class.
=============================================================================*/

#pragma once


/**
 * The results of an ArrangeChildren are always returned as an FArrangedChildren.
 * FArrangedChildren supports a filter that is useful for excluding widgets with unwanted
 * visibilities.
 */
class SLATECORE_API FArrangedChildren
{
	private:
	
	EVisibility VisibilityFilter;
	
	public:

	/**
	 * Construct a new container for arranged children that only accepts children that match the VisibilityFilter.
	 * e.g.
	 *  FArrangedChildren ArrangedChildren( VIS_All ); // Children will be included regardless of visibility
	 *  FArrangedChildren ArrangedChildren( EVisibility::Visible ); // Only visible children will be included
	 *  FArrangedChildren ArrangedChildren( EVisibility::Collapsed | EVisibility::Hidden ); // Only hidden and collapsed children will be included.
	 */
	FArrangedChildren( EVisibility InVisibilityFilter )
	: VisibilityFilter( InVisibilityFilter )
	{
	}

	/** Reverse the order of the arranged children */
	void Reverse()
	{
		int32 LastElementIndex = Array.Num() - 1;
		for (int32 WidgetIndex = 0; WidgetIndex < Array.Num()/2; ++WidgetIndex )
		{
			Array.Swap( WidgetIndex, LastElementIndex - WidgetIndex );
		}
	}

	/**
	 * Add an arranged widget (i.e. widget and its resulting geometry) to the list of Arranged children.
	 *
	 * @param VisibilityOverride   The arrange function may override the visibility of the widget for the purposes
	 *                             of layout or performance (i.e. prevent redundant call to Widget->GetVisibility())
	 * @param InWidgetGeometry     The arranged widget (i.e. widget and its geometry)
	 */
	void AddWidget( EVisibility VisibilityOverride, const FArrangedWidget& InWidgetGeometry );
	
	/**
	 * Add an arranged widget (i.e. widget and its resulting geometry) to the list of Arranged children
	 * based on the the visibility filter and the arranged widget's visibility
	 */
	void AddWidget( const FArrangedWidget& InWidgetGeometry );

	EVisibility GetFilter() const
	{
		return VisibilityFilter;
	}

	bool Accepts( EVisibility InVisibility ) const
	{
		return 0 != (InVisibility.Value & VisibilityFilter.Value);
	}

	// We duplicate parts of TArray's interface here!
	// Inheriting from "public TArray<FArrangedWidget>"
	// saves us this boilerplate, but causes instantiation of
	// many template combinations that we do not want.

	private:
	/** Internal representation of the array widgets */
	TArray<FArrangedWidget> Array;

	public:

	TArray<FArrangedWidget>& GetInternalArray()
	{
		return Array;
	}

	const TArray<FArrangedWidget>& GetInternalArray() const
	{
		return Array;
	}

	int32 Num() const
	{
		return Array.Num();
	}

	const FArrangedWidget& operator()(int32 Index) const
	{
		return Array[Index];
	}

	FArrangedWidget& operator()(int32 Index)
	{
		return Array[Index];
	}

	const FArrangedWidget& Last() const
	{
		return Array.Last();
	}

	FArrangedWidget& Last()
	{
		return Array.Last();
	}

	int32 FindItemIndex(const FArrangedWidget& ItemToFind ) const
	{
		return Array.Find(ItemToFind);
	}

	void Remove( int32 Index, int32 Count=1 )
	{
		Array.RemoveAt(Index, Count);
	}

	void Append( const FArrangedChildren& Source )
	{
		Array.Append( Source.Array );
	}

	void Empty()
	{
		Array.Empty();
	}

};
