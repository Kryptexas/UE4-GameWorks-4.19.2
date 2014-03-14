// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


FArrangedWidget FWidgetPath::FindArrangedWidget( TSharedRef<const SWidget> WidgetToFind ) const
{
	for(int32 WidgetIndex = 0; WidgetIndex < Widgets.Num(); ++WidgetIndex)
	{
		if ( Widgets(WidgetIndex).Widget == WidgetToFind )
		{
			return Widgets(WidgetIndex);
		}
	}

	return FArrangedWidget( SNullWidget::NullWidget, FGeometry() );
}


/** Matches any widget that is focusable */
struct FFocusableWidgetMatcher
{
	bool IsMatch( const TSharedRef<const SWidget>& InWidget ) const
	{
		return InWidget->IsEnabled() && InWidget->SupportsKeyboardFocus();
	}
};

/**
 * Move focus either forward on backward in the path level specified by PathLevel.
 * That is, this movement of focus will modify the subtree under Widgets(PathLevel).
 *
 * @param PathLevel       The level in this WidgetPath whose focus to move.
 * @param MoveDirectin    Move focus forward or backward?
 *
 * @return true if the focus moved successfully, false if we were unable to move focus
 */
bool FWidgetPath::MoveFocus(int32 PathLevel, EFocusMoveDirection::Type MoveDirection)
{
	const int32 MoveDirectionAsInt = (MoveDirection == EFocusMoveDirection::Next)
		? +1
		: -1;


	if ( PathLevel == Widgets.Num()-1 )
	{
		// We are the currently focused widget because we are at the very bottom of focus path.
		if (MoveDirection == EFocusMoveDirection::Next)
		{
			// EFocusMoveDirection::Next implies descend, so try to find a focusable descendant.
			return ExtendPathTo( FFocusableWidgetMatcher() );
		}
		else
		{
			// EFocusMoveDirection::Previous implies move focus up a level.
			return false;
		}
		
	}
	else if ( Widgets.Num() > 1 )
	{
		// We are not the last widget in the path.
		// GOAL: Look for a focusable descendant to the left or right of the currently focused path.
	
		// Arrange the children so we can iterate through them regardless of widget type.
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Widgets(PathLevel).Widget->ArrangeChildren( Widgets(PathLevel).Geometry, ArrangedChildren );

		// Find the currently focused child among the children.
		int32 FocusedChildIndex = ArrangedChildren.FindItemIndex( Widgets(PathLevel+1) );
		FocusedChildIndex = (FocusedChildIndex) % ArrangedChildren.Num() + MoveDirectionAsInt;

		// Now actually search for the widget.
		for( ; FocusedChildIndex < ArrangedChildren.Num() && FocusedChildIndex >= 0; FocusedChildIndex += MoveDirectionAsInt )
		{
			// Neither disabled widgets nor their children can be focused.
			if ( ArrangedChildren(FocusedChildIndex).Widget->IsEnabled() )
			{
				// Look for a focusable descendant.
				FArrangedChildren PathToFocusableChild = GeneratePathToWidget( FFocusableWidgetMatcher(), ArrangedChildren(FocusedChildIndex), MoveDirection );
				// Either we found a focusable descendant, or an immediate child that is focusable.
				const bool bFoundNextFocusable = ( PathToFocusableChild.Num() > 0 ) || ArrangedChildren(FocusedChildIndex).Widget->SupportsKeyboardFocus();
				if ( bFoundNextFocusable )
				{
					// We found the next focusable widget, so make this path point at the new widget by:
					// First, truncating the FocusPath up to the current level (i.e. PathLevel).
					Widgets.Remove( PathLevel+1, Widgets.Num()-PathLevel-1 );
					// Second, add the immediate child that is focus or whose descendant is focused.
					Widgets.AddWidget( ArrangedChildren(FocusedChildIndex) );
					// Add path to focused descendants if any.
					Widgets.Append( PathToFocusableChild );
					// We successfully moved focus!
					return true;
				}
			}
		}		
		
	}

	return false;
}

/** Construct a weak widget path from a widget path. Defaults to an invalid path. */
FWeakWidgetPath::FWeakWidgetPath( const FWidgetPath& InWidgetPath )
: Window( InWidgetPath.TopLevelWindow )
{
	for( int32 WidgetIndex = 0; WidgetIndex < InWidgetPath.Widgets.Num(); ++WidgetIndex )
	{
		Widgets.Add( TWeakPtr<SWidget>( InWidgetPath.Widgets(WidgetIndex).Widget ) );
	}
}

/** Make a non-weak WidgetPath out of this WeakWidgetPath. Do this by computing all the relevant geometries and converting the weak pointers to TSharedPtr. */	
FWidgetPath FWeakWidgetPath::ToWidgetPath(EInterruptedPathHandling::Type InterruptedPathHandling) const
{
	FWidgetPath WidgetPath;
	ToWidgetPath( WidgetPath, InterruptedPathHandling );
	return WidgetPath;
}

FWeakWidgetPath::EPathResolutionResult::Result FWeakWidgetPath::ToWidgetPath( FWidgetPath& WidgetPath, EInterruptedPathHandling::Type InterruptedPathHandling ) const
{
	FArrangedChildren PathWithGeometries(EVisibility::Visible);
	TArray< TSharedPtr<SWidget> > WidgetPtrs;
	
	// Convert the weak pointers into shared pointers because we are about to do something with this path instead of just observe it.
	TSharedPtr<SWindow> TopLevelWindowPtr = Window.Pin();
	for( TArray< TWeakPtr<SWidget> >::TConstIterator SomeWeakWidgetPtr( Widgets ); SomeWeakWidgetPtr; ++SomeWeakWidgetPtr )
	{
		WidgetPtrs.Add( SomeWeakWidgetPtr->Pin() );
	}	
	
	// The path can get interrupted if some subtree of widgets disappeared, but we still maintain weak references to it.
	bool bPathUninterrupted = false;

	// For each widget in the path compute the geometry. We are able to do this starting with the top-level window because it knows its own geometry.
	if ( TopLevelWindowPtr.IsValid() )
	{
		bPathUninterrupted = true;

		FGeometry ParentGeometry = TopLevelWindowPtr->GetWindowGeometryInScreen();
		PathWithGeometries.AddWidget( FArrangedWidget( TopLevelWindowPtr.ToSharedRef(), ParentGeometry ) );
		
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		
		// For every widget in the vertical slice...
		for( int32 WidgetIndex = 0; bPathUninterrupted && WidgetIndex < WidgetPtrs.Num()-1; ++WidgetIndex )
		{
			TSharedPtr<SWidget> CurWidget = WidgetPtrs[WidgetIndex];

			bool bFoundChild = false;
			if ( CurWidget.IsValid() )
			{
				// Arrange the widget's children to find their geometries.
				ArrangedChildren.Empty();
				CurWidget->ArrangeChildren(ParentGeometry, ArrangedChildren);
				
				// Find the next widget in the path among the arranged children.
				for( int32 SearchIndex = 0; !bFoundChild && SearchIndex < ArrangedChildren.Num(); ++SearchIndex )
				{						
					if ( ArrangedChildren(SearchIndex).Widget == WidgetPtrs[WidgetIndex+1] )
					{
						bFoundChild = true;
						// Remember the widget and the associated geometry.
						PathWithGeometries.AddWidget( ArrangedChildren(SearchIndex) );
						// The next child in the vertical slice will be arranged with respect to its parent's geometry.
						ParentGeometry = ArrangedChildren(SearchIndex).Geometry;
					}
				}
			}

			bPathUninterrupted = bFoundChild;
			if (!bFoundChild && InterruptedPathHandling == EInterruptedPathHandling::ReturnInvalid )
			{
				return EPathResolutionResult::Truncated;
			}
		}			
	}
	
	WidgetPath = FWidgetPath( TopLevelWindowPtr, PathWithGeometries );
	return bPathUninterrupted ? EPathResolutionResult::Live : EPathResolutionResult::Truncated;
}

bool FWeakWidgetPath::ContainsWidget( const TSharedRef< const SWidget >& SomeWidget ) const
{
	for ( int32 WidgetIndex=0; WidgetIndex<Widgets.Num(); ++WidgetIndex )
	{
		if (Widgets[WidgetIndex].Pin() == SomeWidget)
		{
			return true;
		}
	}

	return false;
}

/**
 * @param MoveDirection      Direction in which to move the focus.
 * 
 * @return The new focus path.
 */
FWidgetPath FWeakWidgetPath::ToNextFocusedPath(EFocusMoveDirection::Type MoveDirection)
{
	// Make a copy of the focus path. We will mutate it until it meets the necessary requirements.
	FWidgetPath NewFocusPath = this->ToWidgetPath();
	TSharedPtr<SWidget> CurrentlyFocusedWidget = this->Widgets.Last().Pin();

	bool bMovedFocus = false;
	// Attempt to move the focus starting at the leafmost widget and bubbling up to the root (i.e. the window)
	for (int32 FocusNodeIndex=NewFocusPath.Widgets.Num()-1; !bMovedFocus && FocusNodeIndex >= 0; --FocusNodeIndex)
	{
		bMovedFocus = NewFocusPath.MoveFocus( FocusNodeIndex, MoveDirection );
	}

	return NewFocusPath;
}


