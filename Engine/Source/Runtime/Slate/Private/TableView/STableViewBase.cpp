// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "SListPanel.h"

/** Create the child widgets that comprise the list */
void STableViewBase::ConstructChildren( const TAttribute<float>& InItemWidth, const TAttribute<float>& InItemHeight, const TSharedPtr<SHeaderRow>& InHeaderRow, const TSharedPtr<SScrollBar>& InScrollBar )
{
	HeaderRow = InHeaderRow;

	// If the user provided a scrollbar, we do not need to make one of our own.
	if (InScrollBar.IsValid())
	{
		ScrollBar = InScrollBar;
		ScrollBar->SetOnUserScrolled( FOnUserScrolled::CreateSP(this, &STableViewBase::ScrollBar_OnUserScrolled) );
	}
	
	
	TSharedRef<SWidget> ListAndScrollbar = (!ScrollBar.IsValid())
		// We need to make our own scrollbar
		? StaticCastSharedRef<SWidget>
		(
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1)
			[
				SAssignNew( ItemsPanel, SListPanel )
				.ItemWidth(InItemWidth)
				.ItemHeight( InItemHeight )
				.NumDesiredItems( this, &STableViewBase::GetNumItemsBeingObserved )
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( FOptionalSize( 16 ) )
				[
					SAssignNew(ScrollBar, SScrollBar)
					.OnUserScrolled( this, &STableViewBase::ScrollBar_OnUserScrolled )
				]
			]
		)
		// The user provided us with a scrollbar; we will rely on it.
		: StaticCastSharedRef<SWidget>
		(
			SAssignNew( ItemsPanel, SListPanel )
			.ItemWidth(InItemWidth)
			.ItemHeight( InItemHeight )
			.NumDesiredItems( this, &STableViewBase::GetNumItemsBeingObserved )
		);

	if (InHeaderRow.IsValid())
	{
		InHeaderRow->SetAssociatedVerticalScrollBar( ScrollBar.ToSharedRef(), 16 );

		this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				InHeaderRow.ToSharedRef()
			]
			+SVerticalBox::Slot()
			.FillHeight(1)
			[
				ListAndScrollbar
			]
		];
	}
	else
	{
		this->ChildSlot
		[
			ListAndScrollbar
		];
	}
	
}

/**
 * See SWidget::SupportsKeyboardFocus().
 *
 * @return  True if this widget can take keyboard focus
 */
bool STableViewBase::SupportsKeyboardFocus() const
{
	// The ListView is focusable.
	return true;
}


void STableViewBase::OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent )
{
	bShowSoftwareCursor = false;
}


void STableViewBase::OnMouseCaptureLost()
{
	bShowSoftwareCursor = false;
}


struct FEndOfListResult
{
	FEndOfListResult( float InOffset, float InItemsAboveView )
	: OffsetFromEndOfList(InOffset)
	, ItemsAboveView(InItemsAboveView)
	{

	}

	float OffsetFromEndOfList;
	float ItemsAboveView;
};

static FEndOfListResult ComputeOffsetForEndOfList( const FGeometry& ListPanelGeometry, const FChildren& ListPanelChildren )
{
	float OffsetFromEndOfList = 0.0f;
	float AvailableSpace = ListPanelGeometry.Size.Y;
	float ItemsAboveView = 0.0f;
	for ( int ChildIndex=ListPanelChildren.Num()-1; ChildIndex >= 0; --ChildIndex )
	{
		const float CurChildHeight = ListPanelChildren.GetChildAt(ChildIndex)->GetDesiredSize().Y;
		if (AvailableSpace == 0)
		{
			ItemsAboveView ++;
		}

		if ( CurChildHeight < AvailableSpace )
		{
			// This whole child fits
			OffsetFromEndOfList += 1;
			AvailableSpace -= CurChildHeight;
		}
		else
		{
			OffsetFromEndOfList += AvailableSpace / CurChildHeight;
			ItemsAboveView += (CurChildHeight - AvailableSpace)/CurChildHeight;
			AvailableSpace = 0;
		}
	}

	return FEndOfListResult( OffsetFromEndOfList, ItemsAboveView );
}

/**
 * See SWidget::Tick()
 *
 * @param  AllottedGeometry The space allotted for this widget
 * @param  InCurrentTime  Current absolute real time
 * @param  InDeltaTime  Real time passed since last tick
 */
void STableViewBase::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (ItemsPanel.IsValid())
	{
		if ( !IsRightClickScrolling() )
		{
			this->InertialScrollManager.UpdateScrollVelocity(InDeltaTime);

			const float ScrollVelocity = this->InertialScrollManager.GetScrollVelocity();
			if ( ScrollVelocity != 0.f )
			{
				this->ScrollBy(AllottedGeometry, ScrollVelocity * InDeltaTime);
			}
		}

		FGeometry PanelGeometry = FindChildGeometry( AllottedGeometry, ItemsPanel.ToSharedRef() );
		if ( bItemsNeedRefresh || PanelGeometryLastTick.Size != PanelGeometry.Size)
		{
			PanelGeometryLastTick = PanelGeometry;

			// We never create the ItemsPanel if the user did not specify all the parameters required to successfully make a list.
			ScrollIntoView( PanelGeometry );

			const FReGenerateResults ReGenerateResults = ReGenerateItems( PanelGeometry );
			LastGenerateResults = ReGenerateResults;

			const int32 NumItemsBeingObserved = GetNumItemsBeingObserved();

			const bool bEnoughRoomForAllItems = ReGenerateResults.ExactNumWidgetsOnScreen >= NumItemsBeingObserved;
			if (bEnoughRoomForAllItems)
			{
				// We can show all the items, so make sure there is no scrolling.
				ScrollOffset = 0;
			}
			else if ( ReGenerateResults.bGeneratedPastLastItem )
			{
				ScrollOffset = ReGenerateResults.NewScrollOffset;
			}
			
			ScrollOffset = FMath::Max(0.0, ScrollOffset);
			ItemsPanel->SmoothScrollOffset( FMath::Fractional(ScrollOffset / GetNumItemsWide()) );

			UpdateSelectionSet();

			// Update scrollbar
			{
				// The thumb size is whatever fraction of the items we are currently seeing (including partially seen items).
				// e.g. if we are seeing 0.5 of the first generated widget and 0.75 of the last widget, that's 1.25 widgets.
				const double ThumbSizeFraction = ReGenerateResults.ExactNumWidgetsOnScreen / NumItemsBeingObserved;
				const double OffsetFraction = ScrollOffset / NumItemsBeingObserved;
				ScrollBar->SetState( OffsetFraction, ThumbSizeFraction );
			}

			NotifyItemScrolledIntoView();

			bWasAtEndOfList = ScrollBar->DistanceFromBottom() <= 0.f ? true : false;

			bItemsNeedRefresh = false;
			ItemsPanel->SetRefreshPending(false);
		}
	}
}

/**
 * Invoked by the scrollbar when the user scrolls.
 *
 * @param InScrollOffsetFraction  The location to which the user scrolled as a fraction (between 0 and 1) of total height of the content.
 */
void STableViewBase::ScrollBar_OnUserScrolled( float InScrollOffsetFraction )
{
	const double ClampedScrollOffsetInItems = FMath::Clamp<double>( InScrollOffsetFraction, 0.0, 1.0 )* GetNumItemsBeingObserved();
	ScrollTo( ClampedScrollOffsetInItems );
}

/**
 * See SWidget::OnMouseButtonDown()
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply STableViewBase::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Zero the scroll velocity so the list stops immediately on mouse down, even if the user does not drag
	this->InertialScrollManager.ClearScrollVelocity();

	if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && ScrollBar->IsNeeded() )
	{
		AmountScrolledWhileRightMouseDown = 0;

		// NOTE: We don't bother capturing the mouse, unless the user starts dragging a few pixels (see the
		// mouse move handling here.)  This is important so that the item row has a chance to select
		// items when the right mouse button is released.  Just keep in mind that you might not get
		// an OnMouseButtonUp event for the right mouse button if the user moves off of the table before
		// they reach our scroll threshold
		return FReply::Handled();
	}
	return FReply::Unhandled();			
}

/**
 * See SWidget::OnMouseButtonUp()
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply STableViewBase::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		OnRightMouseButtonUp( MouseEvent.GetScreenSpacePosition() );

		FReply Reply = FReply::Handled().ReleaseMouseCapture();
		bShowSoftwareCursor = false;

		// If we have mouse capture, snap the mouse back to the closest location that is within the list's bounds
		if ( HasMouseCapture() )
		{
			FSlateRect ListScreenSpaceRect = MyGeometry.GetClippingRect();
			FVector2D CursorPosition = MyGeometry.LocalToAbsolute( SoftwareCursorPosition );

			FIntPoint BestPositionInList(
				FMath::Round( FMath::Clamp( CursorPosition.X, ListScreenSpaceRect.Left, ListScreenSpaceRect.Right ) ),
				FMath::Round( FMath::Clamp( CursorPosition.Y, ListScreenSpaceRect.Top, ListScreenSpaceRect.Bottom ) )
				);

			Reply.SetMousePos(BestPositionInList);
		}

		return Reply;
	}
	return FReply::Unhandled();
}


/**
 * See SWidget::OnMouseMove()
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply STableViewBase::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{	
	if( MouseEvent.IsMouseButtonDown( EKeys::RightMouseButton ) )
	{
		const float ScrollByAmount = MouseEvent.GetCursorDelta().Y;
		// If scrolling with the right mouse button, we need to remember how much we scrolled.
		// If we did not scroll at all, we will bring up the context menu when the mouse is released.
		AmountScrolledWhileRightMouseDown += FMath::Abs( ScrollByAmount );

		// Has the mouse moved far enough with the right mouse button held down to start capturing
		// the mouse and dragging the view?
		if( IsRightClickScrolling() )
		{
			this->InertialScrollManager.AddScrollSample(-ScrollByAmount, FPlatformTime::Seconds());
			const float AmountScrolled = this->ScrollBy( MyGeometry, -ScrollByAmount );

			FReply Reply = FReply::Handled();

			// Capture the mouse if we need to
			if( FSlateApplication::Get().GetMouseCaptor().Get() != this )
			{
				Reply.CaptureMouse( AsShared() ).UseHighPrecisionMouseMovement( AsShared() );
				SoftwareCursorPosition = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );
				bShowSoftwareCursor = true;
			}

			// Check if the mouse has moved.
			if( AmountScrolled != 0 )
			{
				SoftwareCursorPosition.Y += MouseEvent.GetCursorDelta().Y;
			}

			return Reply;
		}
	}

	return FReply::Unhandled();
}


void STableViewBase::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	if( FSlateApplication::Get().GetMouseCaptor().Get() != this )
	{
		// No longer scrolling (unless we have mouse capture)
		AmountScrolledWhileRightMouseDown = 0;
	}
}


/**
 * See SWidget::OnMouseWheel()
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param MouseEvent Information about the input event
 *
 * @return Whether the event was handled along with possible requests for the system to take action.
 */
FReply STableViewBase::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if( !MouseEvent.IsControlDown() )
	{
		// Make sure scroll velocity is cleared so it doesn't fight with the mouse wheel input
		this->InertialScrollManager.ClearScrollVelocity();

		const float AmountScrolledInItems = this->ScrollBy( MyGeometry, -MouseEvent.GetWheelDelta()*WheelScrollAmount );
		if (FMath::Abs(AmountScrolledInItems) > 0.0f)
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

/**
 * See SWidget::OnKeyDown().
 *
 * @param MyGeometry The Geometry of the widget receiving the event
 * @param  InKeyboardEvent  Keyboard event
 *
 * @return  Returns whether the event was handled, along with other possible actions
 */
FReply STableViewBase::OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent )
{
	if ( InKeyboardEvent.IsControlDown() && InKeyboardEvent.GetKey() == EKeys::End )
	{
		ScrollOffset = GetNumItemsBeingObserved();
		RequestListRefresh();
		return FReply::Handled();
	}


	return FReply::Unhandled();
}

FCursorReply STableViewBase::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	if ( IsRightClickScrolling() )
	{
		// We hide the native cursor as we'll be drawing the software EMouseCursor::GrabHandClosed cursor
		return FCursorReply::Cursor( EMouseCursor::None );
	}
	else
	{
		return FCursorReply::Unhandled();
	}
}


/** @return The number of Widgets we currently have generated. */
int32 STableViewBase::GetNumGeneratedChildren() const
{
	return (ItemsPanel.IsValid())
		? ItemsPanel->GetChildren()->Num()
		: 0;
}

TSharedPtr<SHeaderRow> STableViewBase::GetHeaderRow() const
{
	return HeaderRow;
}

bool STableViewBase::IsRightClickScrolling() const
{
	return AmountScrolledWhileRightMouseDown >= SlatePanTriggerDistance && this->ScrollBar->IsNeeded();
}

bool STableViewBase::IsUserScrolling() const
{
	bool bUserScroll = this->ScrollBar->IsNeeded() && this->ScrollBar->IsScrolling();
	return bUserScroll || IsRightClickScrolling();
}

void STableViewBase::RequestListRefresh()
{
	bItemsNeedRefresh = true;
	ItemsPanel->SetRefreshPending(true);
}

bool STableViewBase::IsPendingRefresh() const
{
	return bItemsNeedRefresh || ItemsPanel->IsRefreshPending();
}

int32 STableViewBase::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	int32 NewLayerId = SCompoundWidget::OnPaint( AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );

	if( !bShowSoftwareCursor )
	{
		return NewLayerId;
	}

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush(TEXT("SoftwareCursor_Grab"));

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		++NewLayerId,
		AllottedGeometry.ToPaintGeometry( SoftwareCursorPosition - ( Brush->ImageSize / 2 ), Brush->ImageSize ),
		Brush,
		MyClippingRect
		);

	return NewLayerId;
}

STableViewBase::STableViewBase( ETableViewMode::Type InTableViewMode )
	: TableViewMode( InTableViewMode )
	, ScrollOffset( 0 )
	, AmountScrolledWhileRightMouseDown( 0 )
	, LastGenerateResults( 0,0,0,false )
	, bWasAtEndOfList(false)
	, SelectionMode( ESelectionMode::Multi )
	, SoftwareCursorPosition( ForceInitToZero )
	, bShowSoftwareCursor( false )
	, bItemsNeedRefresh( true )
{
}

float STableViewBase::ScrollBy( const FGeometry& MyGeometry, float ScrollByAmountInSlateUnits )
{
	const int32 NumItemsBeingObserved = GetNumItemsBeingObserved();
	const float FractionalScrollOffsetInItems = (ScrollOffset + GetScrollRateInItems() * ScrollByAmountInSlateUnits) / NumItemsBeingObserved;
	const double ClampedScrollOffsetInItems = FMath::Clamp<double>( FractionalScrollOffsetInItems, 0.0, 1.0 ) * NumItemsBeingObserved;
	return ScrollTo( ClampedScrollOffsetInItems );
}

float STableViewBase::ScrollTo( float InScrollOffset )
{
	const float NewScrollOffset = FMath::Clamp( InScrollOffset, 0.0f, (float)GetNumItemsBeingObserved() );
	float AmountScrolled = FMath::Abs( ScrollOffset - NewScrollOffset );
	ScrollOffset = NewScrollOffset;

	RequestListRefresh();	

	if ( bWasAtEndOfList && NewScrollOffset >= ScrollOffset )
	{
		AmountScrolled = 0;
	}

	return AmountScrolled;
}

void STableViewBase::InsertWidget( const TSharedRef<ITableRow> & WidgetToInset )
{
	ItemsPanel->AddSlot(0)
	[
		WidgetToInset->AsWidget()
	];
}

/**
 * Add a widget to the view.
 *
 * @param WidgetToAppend   Widget to append to the view.
 */
void STableViewBase::AppendWidget( const TSharedRef<ITableRow>& WidgetToAppend )
{
	ItemsPanel->AddSlot()
	[
		WidgetToAppend->AsWidget()
	];
}

/**
 * Remove all the widgets from the view.
 */
void STableViewBase::ClearWidgets()
{
	ItemsPanel->ClearItems();
}

/**
 * Get the uniform item width.
 */
float STableViewBase::GetItemWidth() const
{
	// Casting to a STilePanel here because we constructed it in Construct()
	return ItemsPanel->GetItemWidth() + ItemsPanel->GetItemPadding(PanelGeometryLastTick);
}

/**
 * Get the uniform item height that is enforced by ListViews.
 */
float STableViewBase::GetItemHeight() const
{
	return ItemsPanel->GetItemHeight();
}

float STableViewBase::GetNumLiveWidgets() const
{
	return ItemsPanel->GetChildren()->Num();
}

int32 STableViewBase::GetNumItemsWide() const
{
	return 1;
}

void STableViewBase::OnRightMouseButtonUp(const FVector2D& SummonLocation)
{
	const bool bShouldOpenContextMenu = !IsRightClickScrolling();
	const bool bContextMenuOpeningBound = OnContextMenuOpening.IsBound();

	if ( bShouldOpenContextMenu && bContextMenuOpeningBound )
	{
		// Get the context menu content. If NULL, don't open a menu.
		TSharedPtr<SWidget> MenuContent = OnContextMenuOpening.Execute();

		if( MenuContent.IsValid() )
		{
			bShowSoftwareCursor = false;
			FSlateApplication::Get().PushMenu( AsShared(), MenuContent.ToSharedRef(), SummonLocation, FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu ) );
		}
	}

	AmountScrolledWhileRightMouseDown = 0;
}

float STableViewBase::GetScrollRateInItems() const
{
	return ( LastGenerateResults.HeightOfGeneratedItems != 0 && LastGenerateResults.ExactNumWidgetsOnScreen != 0)
		// Approximate a consistent scrolling rate based on the average item height.
		? LastGenerateResults.ExactNumWidgetsOnScreen / LastGenerateResults.HeightOfGeneratedItems
		// Scroll 1/2 an item at a time as a default.
		: 0.5f;
}

FVector2D STableViewBase::GetScrollDistance()
{
	return FVector2D( 0, ScrollBar->DistanceFromTop() );
}

FVector2D STableViewBase::GetScrollDistanceRemaining()
{
	return FVector2D( 0, ScrollBar->DistanceFromBottom() );
}

TSharedRef<class SWidget> STableViewBase::GetScrollWidget()
{
	return SharedThis(this);
}
