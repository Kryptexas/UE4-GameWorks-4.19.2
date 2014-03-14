
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MatineeTrackView.cpp: Matinee Track View/Window functionality
=============================================================================*/

#include "MatineeModule.h"
#include "Matinee.h"

#include "Runtime/Engine/Public/InterpolationHitProxy.h"
#include "Runtime/Engine/Public/Slate/SceneViewport.h"


/*-----------------------------------------------------------------------------
 SMatineeVCHolder
 -----------------------------------------------------------------------------*/

void SMatineeViewport::Construct(const FArguments& InArgs, TWeakPtr<FMatinee> InMatinee)
{
	this->ChildSlot
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.FillWidth(1)
		[
			SAssignNew(ViewportWidget, SViewport)
			.EnableGammaCorrection(false)
			.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
			.ShowEffectWhenDisabled(false)
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(ScrollBar_Vert, SScrollBar)
			.AlwaysShowScrollbar(true)
			.OnUserScrolled(this, &SMatineeViewport::OnScroll)
		]
	];

	InterpEdVC = MakeShareable(new FMatineeViewportClient(InMatinee.Pin().Get()));

	InterpEdVC->bSetListenerPosition = false;

	InterpEdVC->VisibilityDelegate.BindSP(this, &SMatineeViewport::IsVisible);

	InterpEdVC->SetRealtime(true);

	Viewport = MakeShareable(new FSceneViewport(InterpEdVC.Get(), ViewportWidget));
	InterpEdVC->Viewport = Viewport.Get();

	// The viewport widget needs an interface so it knows what should render
	ViewportWidget->SetViewportInterface( Viewport.ToSharedRef() );
	
	// Setup the initial metrics for the scroll bar
	AdjustScrollBar();

}

SMatineeViewport::~SMatineeViewport()
{
	if (InterpEdVC.IsValid())
	{
		InterpEdVC->Viewport = NULL;
	}
}

/**
 * Returns the Mouse Position in the viewport
 */
FIntPoint SMatineeViewport::GetMousePos()
{
	FIntPoint Pos;
	if (Viewport.IsValid())
	{
		Viewport->GetMousePos(Pos);
	}

	return Pos;
}

/**
 * Updates the scroll bar for the current state of the window's size and content layout.  This should be called
 *  when either the window size changes or the vertical size of the content contained in the window changes.
 */
void SMatineeViewport::AdjustScrollBar()
{
	if( InterpEdVC.IsValid() && ScrollBar_Vert.IsValid() )
	{

		// Grab the height of the client window
		const uint32 ViewportHeight = InterpEdVC->Viewport->GetSizeXY().Y;

		if (ViewportHeight > 0)
		{
			// Compute scroll bar layout metrics
			uint32 ContentHeight = InterpEdVC->ComputeGroupListContentHeight();
			uint32 ContentBoxHeight =	InterpEdVC->ComputeGroupListBoxHeight( ViewportHeight );


			// The current scroll bar position
			float ScrollBarPos = -((float)InterpEdVC->ThumbPos_Vert) / ((float)ContentHeight);

			// The thumb size is the number of 'scrollbar units' currently visible
			float NewScrollBarThumbSize = ((float)ContentBoxHeight) / ((float)ContentHeight);   //ContentBoxHeight;

			if (NewScrollBarThumbSize > 1.f)
			{
				InterpEdVC->ThumbPos_Vert = 0;
				NewScrollBarThumbSize = 1.f;
			}

			ScrollBarThumbSize = NewScrollBarThumbSize;
			OnScroll(ScrollBarPos);
		}
	}
}

void SMatineeViewport::OnScroll( float InScrollOffsetFraction )
{
	const float LowerLimit = 1.0f - ScrollBarThumbSize;
	if (InScrollOffsetFraction > LowerLimit)
	{
		InScrollOffsetFraction = LowerLimit;
	}

	if( InterpEdVC.IsValid() && ScrollBar_Vert.IsValid() )
	{
		// Compute scroll bar layout metrics
		uint32 ContentHeight = InterpEdVC->ComputeGroupListContentHeight();

		InterpEdVC->ThumbPos_Vert = -InScrollOffsetFraction * ContentHeight;
		ScrollBar_Vert->SetState(InScrollOffsetFraction, ScrollBarThumbSize);

		// Force it to draw so the view change is seen
		InterpEdVC->Viewport->Invalidate();
		InterpEdVC->Viewport->Draw();
	}
}

bool SMatineeViewport::IsVisible() const
{
	return InterpEdVC.IsValid() && (!InterpEdVC->ParentTab.IsValid() || InterpEdVC->ParentTab.Pin()->IsForeground());
}

void FMatinee::OnToggleDirectorTimeline()
{
	if (DirectorTrackWindow.IsValid() && DirectorTrackWindow->InterpEdVC.IsValid())
	{
		DirectorTrackWindow->InterpEdVC->bWantTimeline = !DirectorTrackWindow->InterpEdVC->bWantTimeline;
		DirectorTrackWindow->InterpEdVC->Viewport->Invalidate();
		DirectorTrackWindow->InterpEdVC->Viewport->Draw();
	}
}

bool FMatinee::IsDirectorTimelineToggled()
{
	if (DirectorTrackWindow.IsValid() && DirectorTrackWindow->InterpEdVC.IsValid())
	{
		return DirectorTrackWindow->InterpEdVC->bWantTimeline;
	}

	return false;
}

void FMatinee::BuildTrackWindow()
{	
	TWeakPtr<FMatinee> MatineePtr = SharedThis(this);

	TrackWindow = SNew(SMatineeViewport, MatineePtr);
	DirectorTrackWindow = SNew(SMatineeViewport, MatineePtr);

	// Start off with the window unsplit, showing only the regular track window
	UpdateDirectorTrackWindowVisibility();

	// Setup track window defaults
	TrackWindow->InterpEdVC->bIsDirectorTrackWindow = false;
	TrackWindow->InterpEdVC->bWantFilterTabs = true;
	TrackWindow->InterpEdVC->bWantTimeline = true;

	DirectorTrackWindow->InterpEdVC->bIsDirectorTrackWindow = true;
	DirectorTrackWindow->InterpEdVC->bWantTimeline = true;
}
