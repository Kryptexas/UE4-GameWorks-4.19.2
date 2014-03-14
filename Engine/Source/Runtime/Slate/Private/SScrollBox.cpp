// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "Slate.h"
#include "LayoutUtils.h"


SScrollBox::FSlot& SScrollBox::Slot()
{
	return *(new SScrollBox::FSlot());
}


class SScrollPanel : public SPanel
{
public:
	
	SLATE_BEGIN_ARGS(SScrollPanel)
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}
	SLATE_END_ARGS()



	void Construct( const FArguments& InArgs, const TArray<SScrollBox::FSlot*>& InSlots )
	{
		PhysicalOffset = 0;
		Children.Reserve( InSlots.Num() );
		for ( int32 SlotIndex=0; SlotIndex < InSlots.Num(); ++SlotIndex )
		{
			Children.Add( InSlots[ SlotIndex ] );
		}
	}



public:

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE
	{
		
		float CurChildOffset = -PhysicalOffset;

		for( int32 SlotIndex=0; SlotIndex < Children.Num(); ++SlotIndex )
		{
			const SScrollBox::FSlot& ThisSlot = Children[SlotIndex];
			const EVisibility ChildVisibility = ThisSlot.Widget->GetVisibility();

			if ( ChildVisibility != EVisibility::Collapsed )
			{
				const FMargin& ThisPadding = ThisSlot.SlotPadding.Get();
				const FVector2D& WidgetDesiredSize = ThisSlot.Widget->GetDesiredSize();
				const float ThisSlotDesiredHeight = WidgetDesiredSize.Y + ThisSlot.SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();

				// Figure out the size and local position of the child within the slot
				// There is no vertical alignment, because it does not make sense in a panel where items are stacked end-to-end
				AlignmentArrangeResult XAlignmentResult = AlignChild<Orient_Horizontal>( AllottedGeometry.Size.X, ThisSlot, ThisPadding );

				ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( ThisSlot.Widget, FVector2D(XAlignmentResult.Offset, CurChildOffset + ThisPadding.Top), FVector2D(XAlignmentResult.Size, WidgetDesiredSize.Y) ) );
				CurChildOffset += ThisSlotDesiredHeight;
			}
		}


	}



	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		FVector2D ThisDesiredSize = FVector2D::ZeroVector;
		for(int32 SlotIndex=0; SlotIndex < Children.Num(); ++SlotIndex )
		{
			const SScrollBox::FSlot& ThisSlot = Children[SlotIndex];
			const FVector2D ChildDesiredSize = ThisSlot.Widget->GetDesiredSize();
			ThisDesiredSize.X  = FMath::Max(ChildDesiredSize.X, ThisDesiredSize.X);
			ThisDesiredSize.Y += ChildDesiredSize.Y + ThisSlot.SlotPadding.Get().GetTotalSpaceAlong<Orient_Vertical>();
		}

		return ThisDesiredSize;
	}



	virtual FChildren* GetChildren() OVERRIDE
	{
		return &Children;
	}


	float PhysicalOffset;
	TPanelChildren<SScrollBox::FSlot> Children;	
	
};



void SScrollBox::Construct( const FArguments& InArgs )
{
	check(InArgs._Style);

	DesiredScrollOffset = 0;
	bIsScrolling = false;
	bAnimateScroll = false;
	AmountScrolledWhileRightMouseDown = 0;
	bShowSoftwareCursor = false;
	SoftwareCursorPosition = FVector2D::ZeroVector;

	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.Padding( FMargin(0.0f, 0.0f, 0.0f, 1.0f) )
			[
				// Scroll panel that presents the scrolled content
				SAssignNew( ScrollPanel, SScrollPanel, InArgs.Slots )
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			[
				// Shadow: Hint to scroll up
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.ColorAndOpacity(this, &SScrollBox::GetTopShadowOpacity)
				.Image( &InArgs._Style->TopShadowBrush )	
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			[
				// Shadow: a hint to scroll down
				SNew(SImage)
				.Visibility(EVisibility::HitTestInvisible)
				.ColorAndOpacity(this, &SScrollBox::GetBottomShadowOpacity)
				.Image( &InArgs._Style->BottomShadowBrush )	
			]
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew( ScrollBar, SScrollBar )
			.Thickness( FVector2D(5.0f, 5.0f) )
			.OnUserScrolled( this, &SScrollBox::ScrollBar_OnUserScrolled )
		]
	];
}



/** Adds a slot to SScrollBox */
SScrollBox::FSlot& SScrollBox::AddSlot()
{
	SScrollBox::FSlot& NewSlot = SScrollBox::Slot();
	ScrollPanel->Children.Add( &NewSlot );

	return NewSlot;
}



/** Removes a slot at the specified location */
void SScrollBox::RemoveSlot( const TSharedRef<SWidget>& WidgetToRemove )
{
	TPanelChildren<SScrollBox::FSlot>& Children = ScrollPanel->Children;
	for( int32 SlotIndex=0; SlotIndex < Children.Num(); ++SlotIndex )
	{
		if ( Children[SlotIndex].Widget == WidgetToRemove )
		{
			Children.RemoveAt(SlotIndex);
			return;
		}
	}
}

void SScrollBox::ClearChildren()
{
	ScrollPanel->Children.Empty();
}

bool SScrollBox::IsRightClickScrolling() const
{
	return AmountScrolledWhileRightMouseDown >= SlatePanTriggerDistance && this->ScrollBar->IsNeeded();
}

void SScrollBox::SetScrollOffset( float NewScrollOffset )
{
	DesiredScrollOffset = NewScrollOffset;
}

void SScrollBox::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if ( !IsRightClickScrolling() )
	{
		this->InertialScrollManager.UpdateScrollVelocity(InDeltaTime);

		const float ScrollVelocity = this->InertialScrollManager.GetScrollVelocity();
		if ( ScrollVelocity != 0.f )
		{
			this->ScrollBy(AllottedGeometry, ScrollVelocity * InDeltaTime, true);
		}
	}

	const FGeometry ScrollPanelGeometry = FindChildGeometry( AllottedGeometry, ScrollPanel.ToSharedRef() );
	
	const float ContentHeight = ScrollPanel->GetDesiredSize().Y;
	const float ViewFraction = ScrollPanelGeometry.Size.Y / ContentHeight;
	const float ViewOffset = FMath::Clamp<float>( DesiredScrollOffset/ContentHeight, 0.0, 1.0 - ViewFraction );
	
	// Update the scrollbar with the clamped version of the offset
	const float TargetPhysicalOffset = ViewOffset*ScrollPanel->GetDesiredSize().Y;
	bIsScrolling = !FMath::IsNearlyEqual(TargetPhysicalOffset, ScrollPanel->PhysicalOffset, 0.001f);	
	ScrollPanel->PhysicalOffset = (bAnimateScroll)
		? FMath::FInterpTo(ScrollPanel->PhysicalOffset, TargetPhysicalOffset, InDeltaTime, 15.f)
		: TargetPhysicalOffset;
	
	ScrollBar->SetState(ViewOffset, ViewFraction);
	if (!ScrollBar->IsNeeded())
	{
		// We cannot scroll, so ensure that there is no offset.
		ScrollPanel->PhysicalOffset = 0.0f;
	}

}


FReply SScrollBox::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	// Zero the scroll velocity so the panel stops immediately on mouse down, even if the user does not drag
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


FReply SScrollBox::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
	{
		AmountScrolledWhileRightMouseDown = 0;

		FReply Reply = FReply::Handled().ReleaseMouseCapture();
		bShowSoftwareCursor = false;

		// If we have mouse capture, snap the mouse back to the closest location that is within the panel's bounds
		if ( HasMouseCapture() )
		{
			FSlateRect PanelScreenSpaceRect = MyGeometry.GetClippingRect();
			FVector2D CursorPosition = MyGeometry.LocalToAbsolute( SoftwareCursorPosition );

			FIntPoint BestPositionInPanel(
				FMath::Round( FMath::Clamp( CursorPosition.X, PanelScreenSpaceRect.Left, PanelScreenSpaceRect.Right ) ),
				FMath::Round( FMath::Clamp( CursorPosition.Y, PanelScreenSpaceRect.Top, PanelScreenSpaceRect.Bottom ) )
				);

			Reply.SetMousePos(BestPositionInPanel);
		}

		return Reply;
	}
	return FReply::Unhandled();
}


FReply SScrollBox::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
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
			const bool bDidScroll = this->ScrollBy( MyGeometry, -ScrollByAmount );

			FReply Reply = FReply::Handled();

			// Capture the mouse if we need to
			if( FSlateApplication::Get().GetMouseCaptor().Get() != this )
			{
				Reply.CaptureMouse( AsShared() ).UseHighPrecisionMouseMovement( AsShared() );
				SoftwareCursorPosition = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );
				bShowSoftwareCursor = true;
			}

			// Check if the mouse has moved.
			if( bDidScroll )
			{
				SoftwareCursorPosition.Y += MouseEvent.GetCursorDelta().Y;
			}

			return Reply;
		}
	}

	return FReply::Unhandled();
}

void SScrollBox::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	if( FSlateApplication::Get().GetMouseCaptor().Get() != this )
	{
		// No longer scrolling (unless we have mouse capture)
		AmountScrolledWhileRightMouseDown = 0;
	}
}

FReply SScrollBox::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (ScrollBar->IsNeeded())
	{
		// Make sure scroll velocity is cleared so it doesn't fight with the mouse wheel input
		this->InertialScrollManager.ClearScrollVelocity();

		const bool bScrollWasHandled = this->ScrollBy( MyGeometry, -MouseEvent.GetWheelDelta()*WheelScrollAmount );

		return bScrollWasHandled ? FReply::Handled() : FReply::Unhandled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

bool SScrollBox::ScrollBy( const FGeometry& AllottedGeometry, float ScrollAmount, bool InAnimateScroll )
{
	bAnimateScroll = InAnimateScroll;

	const float ContentSize = ScrollPanel->GetDesiredSize().Y;
	const FGeometry ScrollPanelGeometry = FindChildGeometry( AllottedGeometry, ScrollPanel.ToSharedRef() );

	DesiredScrollOffset += ScrollAmount;

	const float ScrollMin = 0.0f, ScrollMax = ContentSize - ScrollPanelGeometry.Size.Y;
	if (DesiredScrollOffset < ScrollMin || DesiredScrollOffset > ScrollMax)
	{
		DesiredScrollOffset = FMath::Clamp( DesiredScrollOffset, ScrollMin, ScrollMax );
		return false;
	}
	return true;
}


FReply SScrollBox::OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled().CaptureMouse( SharedThis(this) );
}

FCursorReply SScrollBox::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
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

int32 SScrollBox::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
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

void SScrollBox::ScrollBar_OnUserScrolled( float InScrollOffsetFraction )
{
	DesiredScrollOffset = InScrollOffsetFraction * ScrollPanel->GetDesiredSize().Y;
}

const float ShadowFadeDistance = 32.0f;

FSlateColor SScrollBox::GetTopShadowOpacity() const
{
	// The shadow should only be visible when the user needs a hint that they can scroll up.
	const float ShadowOpacity = FMath::Clamp( ScrollPanel->PhysicalOffset/ShadowFadeDistance, 0.0f, 1.0f);
	
	return FLinearColor(1.0f, 1.0f, 1.0f, ShadowOpacity);
}

FSlateColor SScrollBox::GetBottomShadowOpacity() const
{
	// The shadow should only be visible when the user needs a hint that they can scroll down.
	const float ShadowOpacity = (ScrollBar->DistanceFromBottom() * ScrollPanel->GetDesiredSize().Y) / ShadowFadeDistance;
	
	return FLinearColor(1.0f, 1.0f, 1.0f, ShadowOpacity);
}
