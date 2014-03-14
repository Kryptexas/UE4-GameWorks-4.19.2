// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "PrimitiveTypes.h"
#include "SWidget.h"

SWidget::SWidget()
	: CreatedInFile( TEXT("") )
	, CreatedOnLine( -1 )
	, Cursor( TOptional<EMouseCursor::Type>() )
	, EnabledState( true )
	, Visibility( EVisibility::Visible )
	, DesiredSize(FVector2D::ZeroVector)
	, ToolTip()
	, bToolTipForceFieldEnabled( false )
	, bIsHovered( false )
{

}

void SWidget::Construct(
	const TAttribute<FString> & InToolTipText ,
	const TSharedPtr<SToolTip> & InToolTip ,
	const TAttribute< TOptional<EMouseCursor::Type> > & InCursor ,
	const TAttribute<bool> & InEnabledState ,
	const TAttribute<EVisibility> & InVisibility,
	const FName& InTag
)
{
	if ( InToolTip.IsValid() )
	{
		// If someone specified a fancy widget tooltip, use it.
		ToolTip = InToolTip;
	}
	else if ( InToolTipText.IsBound() || !(InToolTipText.Get().IsEmpty()) )
	{
		// If someone specified a text binding, make a tooltip out of it
		ToolTip = SNew(SToolTip) .Text( InToolTipText );
	}
	else if( !ToolTip.IsValid() || (ToolTip.IsValid() && ToolTip->IsEmpty()) )
	{	
		// We don't have a tooltip.
		ToolTip.Reset();
	}

	Cursor = InCursor;
	EnabledState = InEnabledState;
	Visibility = InVisibility;
}

FReply SWidget::OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	return FReply::Unhandled();
}

void SWidget::OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
}

void SWidget::OnKeyboardFocusChanging( const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath )
{
}

FReply SWidget::OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnPreviewKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	return FReply::Unhandled();
}


FReply SWidget::OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnPreviewMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

void SWidget::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	bIsHovered = true;
}

void SWidget::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	bIsHovered = false;
}

FReply SWidget::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

FCursorReply SWidget::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	TOptional<EMouseCursor::Type> TheCursor = Cursor.Get();
	return ( TheCursor.IsSet() )
		? FCursorReply::Cursor( TheCursor.GetValue() )
		: FCursorReply::Unhandled();
}

FReply SWidget::OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent )
{
	return FReply::Unhandled();
}

bool SWidget::OnVisualizeTooltip( const TSharedPtr<SWidget>& TooltipContent )
{
	return false;
}

FReply SWidget::OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Unhandled();
}

void SWidget::OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
}

void SWidget::OnDragLeave( const FDragDropEvent& DragDropEvent )
{
}

FReply SWidget::OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnControllerButtonPressed( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnControllerButtonReleased( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnControllerAnalogValueChanged( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnTouchGesture( const FGeometry& MyGeometry, const FPointerEvent& GestureEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnTouchMoved( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent )
{
	return FReply::Unhandled();
}

FReply SWidget::OnMotionDetected( const FGeometry& MyGeometry, const FMotionEvent& MotionEvent )
{
	return FReply::Unhandled();
}

EWindowZone::Type SWidget::GetWindowZoneOverride() const
{
	// No special behavior.  Override this in derived widgets, if needed.
	return EWindowZone::Unspecified;
}

void SWidget::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
}

bool SWidget::OnHitTest( const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition )
{
	return true;
}

void SWidget::TickWidgetsRecursively( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	// Tick this widget
	Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

	// Gather all children, whether they're visible or not.  We need to allow invisible widgets to
	// consider whether they should still be invisible in their tick functions, as well as maintain
	// other state when hidden,
	FArrangedChildren ArrangedChildren(EVisibility::All);
	ArrangeChildren(AllottedGeometry, ArrangedChildren);

	// Recurse!
	for(int32 ChildIndex=0; ChildIndex < ArrangedChildren.Num(); ++ChildIndex)
	{
		FArrangedWidget& SomeChild = ArrangedChildren(ChildIndex);
		SomeChild.Widget->TickWidgetsRecursively( SomeChild.Geometry, InCurrentTime, InDeltaTime );
	}
}


void SWidget::SlatePrepass()	
{
	// Cache child desired sizes first. This widget's desired size is
	// a function of its children's sizes.
	FChildren* MyChildren = this->GetChildren();
	int32 NumChildren = MyChildren->Num();
	for(int32 ChildIndex=0; ChildIndex < NumChildren; ++ChildIndex)
	{
		const TSharedRef<SWidget>& Child = MyChildren->GetChildAt(ChildIndex);
		if ( Child->Visibility.Get() != EVisibility::Collapsed )
		{
			// Recur: Descend down the widget tree.
			Child->SlatePrepass();
		}
	}

	// Cache this widget's desired size.
	CacheDesiredSize();
}


void SWidget::CacheDesiredSize()
{
	// Cache this widget's desired size.
	this->Advanced_SetDesiredSize( this->ComputeDesiredSize() );
}


const FVector2D& SWidget::GetDesiredSize() const
{
	return DesiredSize;
}



bool SWidget::SupportsKeyboardFocus() const
{
	return false;
}

bool SWidget::HasKeyboardFocus() const
{
	return ( FSlateApplication::Get().GetKeyboardFocusedWidget().Get() == this );
}

bool SWidget::HasFocusedDescendants() const
{
	return ( FSlateApplication::Get().HasFocusedDescendants( SharedThis( this ) ) );
}

bool SWidget::HasMouseCapture() const
{
	return ( FSlateApplication::Get().GetMouseCaptor().Get() == this );
}

void SWidget::OnMouseCaptureLost()
{
}

bool SWidget::FindChildGeometries( const FGeometry& MyGeometry, const TSet< TSharedRef<SWidget> >& WidgetsToFind, TMap<TSharedRef<SWidget>, FArrangedWidget>& OutResult ) const
{
	FindChildGeometries_Helper(MyGeometry, WidgetsToFind, OutResult);
	return OutResult.Num() == WidgetsToFind.Num();
}

void SWidget::FindChildGeometries_Helper( const FGeometry& MyGeometry, const TSet< TSharedRef<SWidget> >& WidgetsToFind, TMap<TSharedRef<SWidget>, FArrangedWidget>& OutResult ) const
{
	// Perform a breadth first search!

	FArrangedChildren ArrangedChildren(EVisibility::Visible);
	this->ArrangeChildren( MyGeometry, ArrangedChildren );
	const int32 NumChildren = ArrangedChildren.Num();

	// See if we found any of the widgets on this level.
	for(int32 ChildIndex=0; ChildIndex < NumChildren; ++ChildIndex )
	{
		const FArrangedWidget& CurChild = ArrangedChildren( ChildIndex );
		
		if ( WidgetsToFind.Contains(CurChild.Widget) )
		{
			// We found one of the widgets for which we need geometry!
			OutResult.Add( CurChild.Widget, CurChild );
		}
	}

	// If we have not found all the widgets that we were looking for, descend.
	if ( OutResult.Num() != WidgetsToFind.Num() )
	{
		// Look for widgets among the children.
		for( int32 ChildIndex=0; ChildIndex < NumChildren; ++ChildIndex )
		{
			const FArrangedWidget& CurChild = ArrangedChildren( ChildIndex );
			CurChild.Widget->FindChildGeometries_Helper( CurChild.Geometry, WidgetsToFind, OutResult );
		}	
	}	
}


FGeometry SWidget::FindChildGeometry( const FGeometry& MyGeometry, TSharedRef<SWidget> WidgetToFind ) const
{
	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > WidgetsToFind;
	{
		WidgetsToFind.Add( WidgetToFind );
	}
	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;

	FindChildGeometries( MyGeometry, WidgetsToFind, Result );

	return Result.FindChecked( WidgetToFind ).Geometry;
}

int32 SWidget::FindChildUnderMouse( const FArrangedChildren& Children, const FPointerEvent& MouseEvent )
{
	const FVector2D& AbsoluteCursorLocation = MouseEvent.GetScreenSpacePosition();
	const int32 NumChildren = Children.Num();
	for( int32 ChildIndex=NumChildren-1; ChildIndex >= 0; --ChildIndex )
	{
		const FArrangedWidget& Candidate = Children(ChildIndex);
		const bool bCandidateUnderCursor = 
			// Candidate is physically under the cursor
			Candidate.Geometry.IsUnderLocation( AbsoluteCursorLocation ) && 
			// Candidate actually considers itself hit by this test
			(Candidate.Widget->OnHitTest( Candidate.Geometry, AbsoluteCursorLocation ));

		if (bCandidateUnderCursor)
		{
			return ChildIndex;
		}
	}

	return INDEX_NONE;
}


FString SWidget::ToString() const
{
	return FString::Printf(TEXT("%s [%s(%d)]"), *this->TypeOfWidget.ToString(), *this->CreatedInFile.ToString(), this->CreatedOnLine );
}

FString SWidget::GetTypeAsString() const
{
	return this->TypeOfWidget.ToString();
}

FName SWidget::GetType() const
{
	return this->TypeOfWidget;
}

FString SWidget::GetReadableLocation() const
{
	return FString::Printf(TEXT("%s(%d)"), *this->CreatedInFile.ToString(), this->CreatedOnLine );
}

FString SWidget::GetParsableFileAndLineNumber() const
{
	return FString::Printf(TEXT("%s|%d"), *this->CreatedInFileFullPath.ToString(), this->CreatedOnLine );
}

FSlateColor SWidget::GetForegroundColor() const
{
	static FSlateColor NoColor = FSlateColor::UseForeground();
	return NoColor;
}

void SWidget::SetToolTipText( const TAttribute<FString>& InToolTipText )
{
	ToolTip = SNew(SToolTip) .Text(InToolTipText);
}

void SWidget::SetToolTipText( const FText& InToolTipText )
{
	ToolTip = SNew(SToolTip) .Text(InToolTipText);
}

void SWidget::SetToolTip( const TSharedPtr<SToolTip> & InToolTip )
{
	ToolTip = InToolTip;
}

TSharedPtr<SToolTip> SWidget::GetToolTip()
{
	return ToolTip;
}

void SWidget::OnToolTipClosing()
{
}

void SWidget::EnableToolTipForceField( const bool bEnableForceField )
{
	bToolTipForceFieldEnabled = bEnableForceField;
}


void SWidget::SetCursor( const TAttribute< TOptional<EMouseCursor::Type> >& InCursor )
{
	Cursor = InCursor;
}

void SWidget::SetDebugInfo( const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine )
{
	this->TypeOfWidget = InType;
	this->CreatedInFileFullPath = FName( InFile );
	this->CreatedInFile = FName( *FPaths::GetCleanFilename(InFile) );
	this->CreatedOnLine = OnLine;
}
