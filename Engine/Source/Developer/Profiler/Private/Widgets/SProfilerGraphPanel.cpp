// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProfilerPrivatePCH.h"

// @TODO yrx 2014-04-18 Move SProfilerWindow later
#define LOCTEXT_NAMESPACE "SProfilerGraphPanel"

SProfilerGraphPanel::SProfilerGraphPanel()
	: NumDataPoints( 0 )
	, NumVisiblePoints( 0 )
	, GraphOffset( 0 )
{
}

SProfilerGraphPanel::~SProfilerGraphPanel()
{
	// Remove ourselves from the profiler manager.
	if( FProfilerManager::Get().IsValid() )
	{
		FProfilerManager::Get()->OnTrackedStatChanged().RemoveAll( this );
		FProfilerManager::Get()->OnSessionInstancesUpdated().RemoveAll( this );
		FProfilerManager::Get()->OnViewModeChanged().RemoveAll( this );
	}
}

void SProfilerGraphPanel::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
		.Padding( 2.0f )
		[
			SNew( SHorizontalBox )

			+SHorizontalBox::Slot()
			.FillWidth( 1.0f )
			[
				SNew( SVerticalBox )

				+ SVerticalBox::Slot()
				.FillHeight( 1.0f )
				[
					SAssignNew( DataGraph, SDataGraph )
					.OnGraphOffsetChanged( this, &SProfilerGraphPanel::OnDataGraphGraphOffsetChanged )
					.OnViewModeChanged( this, &SProfilerGraphPanel::DataGraph_OnViewModeChanged )
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew( HorizontalScrollBar, SScrollBar )
					.Orientation( Orient_Horizontal )
					.AlwaysShowScrollbar( true )
					.Visibility( EVisibility::Visible )
					.Thickness( FVector2D( 8.0f, 8.0f ) )
					.OnUserScrolled( this, &SProfilerGraphPanel::HorizontalScrollBar_OnUserScrolled )
				]
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew( VerticalScrollBar, SScrollBar )
				.Orientation( Orient_Vertical )
				.AlwaysShowScrollbar( true )
				.Visibility( EVisibility::Visible )
				.Thickness( FVector2D( 8.0f, 8.0f ) )
				.OnUserScrolled( this, &SProfilerGraphPanel::VerticalScrollBar_OnUserScrolled )
			]
		]	
	];

	HorizontalScrollBar->SetState( 0.0f, 1.0f );
	VerticalScrollBar->SetState( 0.0f, 1.0f );

	// Register ourselves with the profiler manager.
	FProfilerManager::Get()->OnTrackedStatChanged().AddSP( this, &SProfilerGraphPanel::ProfilerManager_OnTrackedStatChanged );
	FProfilerManager::Get()->OnSessionInstancesUpdated().AddSP( this, &SProfilerGraphPanel::ProfilerManager_OnSessionInstancesUpdated );

	FProfilerManager::Get()->OnViewModeChanged().AddSP( this, &SProfilerGraphPanel::ProfilerManager_OnViewModeChanged );
}


void SProfilerGraphPanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	UpdateInternals();
}

void SProfilerGraphPanel::HorizontalScrollBar_OnUserScrolled( float ScrollOffset )
{
	const float ThumbSizeFraction = FMath::Min( (float)NumVisiblePoints / (float)NumDataPoints, 1.0f );
	ScrollOffset = FMath::Min( ScrollOffset, 1.0f-ThumbSizeFraction );
 
	HorizontalScrollBar->SetState( ScrollOffset, ThumbSizeFraction );
 
	GraphOffset = FMath::Trunc( ScrollOffset * NumDataPoints );
	DataGraph->ScrollTo( GraphOffset );

	FProfilerManager::Get()->GetProfilerWindow()->ProfilerMiniView->OnSelectionBoxChanged( GraphOffset, GraphOffset + NumVisiblePoints );
}


void SProfilerGraphPanel::VerticalScrollBar_OnUserScrolled( float ScrollOffset )
{

}


void SProfilerGraphPanel::ProfilerManager_OnTrackedStatChanged( const FTrackedStat& TrackedStat, bool bIsTracked )
{
	if( bIsTracked )
	{
		DataGraph->AddInnerGraph( TrackedStat.StatID, TrackedStat.ColorAverage, TrackedStat.ColorBackground, TrackedStat.ColorExtremes, TrackedStat.CombinedGraphDataSource );
	}
	else
	{
		DataGraph->RemoveInnerGraph( TrackedStat.StatID );
	}
}

void SProfilerGraphPanel::ProfilerManager_OnSessionInstancesUpdated()
{

}

void SProfilerGraphPanel::OnDataGraphGraphOffsetChanged( int32 InGraphOffset )
{
	GraphOffset = InGraphOffset;
	SetScrollBarState();
	FProfilerManager::Get()->GetProfilerWindow()->ProfilerMiniView->OnSelectionBoxChanged( InGraphOffset, InGraphOffset + NumVisiblePoints );
}

void SProfilerGraphPanel::OnMiniViewSelectionBoxChanged( int32 FrameStart, int32 FrameEnd )
{
	GraphOffset = FrameStart;
	SetScrollBarState();
	DataGraph->ScrollTo( GraphOffset );
}

void SProfilerGraphPanel::DataGraph_OnViewModeChanged( EDataGraphViewModes::Type ViewMode )
{
	UpdateInternals();
}


void SProfilerGraphPanel::ProfilerManager_OnViewModeChanged( EProfilerViewMode::Type NewViewMode )
{
	if( NewViewMode == EProfilerViewMode::LineIndexBased )
	{
		HorizontalScrollBar->SetVisibility( EVisibility::Collapsed );
		HorizontalScrollBar->SetEnabled( false );
	}
	else if( NewViewMode == EProfilerViewMode::ThreadViewTimeBased )
	{
		HorizontalScrollBar->SetVisibility( EVisibility::Visible );
		HorizontalScrollBar->SetEnabled( true );
	}

	//DataGraph->SetViewMode( NewViewMode );
}


void SProfilerGraphPanel::UpdateInternals()
{
	NumVisiblePoints = DataGraph->GetNumVisiblePoints();
	NumDataPoints = DataGraph->GetNumDataPoints();

	SetScrollBarState();
	FProfilerManager::Get()->GetProfilerWindow()->ProfilerMiniView->OnSelectionBoxChanged( GraphOffset, GraphOffset + NumVisiblePoints );
	 
	if( FProfilerManager::Get()->IsLivePreview() )
	{
		// Scroll to the end.
		HorizontalScrollBar_OnUserScrolled(1.0f);
	}
}

void SProfilerGraphPanel::SetScrollBarState()
{
	// Note that the maximum offset is 1.0-ThumbSizeFraction.
	// If the user can view 1/3 of the items in a single page, the maximum offset will be ~0.667f
	const float ThumbSizeFraction = FMath::Min( (float)NumVisiblePoints / (float)NumDataPoints, 1.0f );
	const float OffsetFraction = (float)GraphOffset / (float)NumDataPoints;
	HorizontalScrollBar->SetState( OffsetFraction, ThumbSizeFraction );
}

#undef LOCTEXT_NAMESPACE
