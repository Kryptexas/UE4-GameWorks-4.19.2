// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProfilerPrivatePCH.h"

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
		FProfilerManager::Get()->OnTrackedStatChanged().RemoveAll(this);
		FProfilerManager::Get()->OnSessionInstancesUpdated().RemoveAll(this);
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
			SNew( SVerticalBox )

			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SAssignNew(DataGraph,SDataGraph)
				.OnGraphOffsetChanged( this, &SProfilerGraphPanel::DataGraph_OnGraphOffsetChanged )
				.OnViewModeChanged( this, &SProfilerGraphPanel::DataGraph_OnViewModeChanged )
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(ScrollBar,SScrollBar)
				.Orientation( Orient_Horizontal	 )
				.AlwaysShowScrollbar( true )
				.Visibility( EVisibility::Visible )
				.Thickness( FVector2D(8.0f,8.0f) )
				.OnUserScrolled( this, &SProfilerGraphPanel::ScrollBar_OnUserScrolled )
			]
		]	
	];

	ScrollBar->SetState( 0.0f, 1.0f );

	// Register ourselves with the profiler manager.
	FProfilerManager::Get()->OnTrackedStatChanged().AddSP( this, &SProfilerGraphPanel::ProfilerManager_OnTrackedStatChanged );
	FProfilerManager::Get()->OnSessionInstancesUpdated().AddSP( this, &SProfilerGraphPanel::ProfilerManager_OnSessionInstancesUpdated );
}


void SProfilerGraphPanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	UpdateInternals();
}

void SProfilerGraphPanel::ScrollBar_OnUserScrolled( float ScrollOffset )
{
	const float ThumbSizeFraction = FMath::Min( (float)NumVisiblePoints / (float)NumDataPoints, 1.0f );
	ScrollOffset = FMath::Min( ScrollOffset, 1.0f-ThumbSizeFraction );
 
	ScrollBar->SetState( ScrollOffset, ThumbSizeFraction );
 
	GraphOffset = FMath::Trunc( ScrollOffset * NumDataPoints );
	DataGraph->ScrollTo( GraphOffset );
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

void SProfilerGraphPanel::DataGraph_OnGraphOffsetChanged( int InGraphOffset )
{
	GraphOffset = InGraphOffset;
	const float ThumbSizeFraction = FMath::Min( (float)NumVisiblePoints / (float)NumDataPoints, 1.0f );
	const float OffsetFraction = (float)GraphOffset / (float)NumDataPoints;
	ScrollBar->SetState( OffsetFraction, ThumbSizeFraction );
}

void SProfilerGraphPanel::DataGraph_OnViewModeChanged( EDataGraphViewModes::Type ViewMode )
{
	UpdateInternals();
}

void SProfilerGraphPanel::UpdateInternals()
{
	NumVisiblePoints = DataGraph->GetNumVisiblePoints();
	NumDataPoints = DataGraph->GetNumDataPoints();

	// Note that the maximum offset is 1.0-ThumbSizeFraction.
	// If the user can view 1/3 of the items in a single page, the maximum offset will be ~0.667f
	const float ThumbSizeFraction = FMath::Min( (float)NumVisiblePoints / (float)NumDataPoints, 1.0f );
	const float OffsetFraction = (float)GraphOffset / (float)NumDataPoints;
		
	const float Distance = ScrollBar->DistanceFromBottom();
	ScrollBar->SetState( OffsetFraction, ThumbSizeFraction );
	 
	if( FProfilerManager::Get()->IsLivePreview() )
	{
		// Scroll to the end.
		ScrollBar_OnUserScrolled(1.0f);
	}
}

#undef LOCTEXT_NAMESPACE
