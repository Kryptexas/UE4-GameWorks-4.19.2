// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProfilerPrivatePCH.h"

/*-----------------------------------------------------------------------------
	FProfilerStat, FProfilerGroup
-----------------------------------------------------------------------------*/

FProfilerStat FProfilerStat::Default;
FProfilerGroup FProfilerGroup::Default;

FProfilerStat::FProfilerStat( const uint32 InStatID /*= 0 */ ) 
	: _Name( TEXT("(Stat-Default)") )
	, _OwningGroupPtr( FProfilerGroup::GetDefaultPtr() )
	, _ID( InStatID )
	, _Type( EProfilerSampleTypes::InvalidOrMax )
{}

/*-----------------------------------------------------------------------------
	FProfilerSession
-----------------------------------------------------------------------------*/

FProfilerSession::FProfilerSession( const ISessionInstanceInfoPtr InSessionInstanceInfo )
	: ClientStatMetadata( nullptr )
	, bRequestStatMetadataUpdate( false )
	, bLastPacket( false )
	, StatMetaDataSize( 0 )
	, DataProvider( MakeShareable( new FArrayDataProvider() ) )
	, StatMetaData( MakeShareable( new FProfilerStatMetaData() ) )

	, OnTick( FTickerDelegate::CreateRaw(this, &FProfilerSession::HandleTicker) )

	, EventGraphDataTotal( MakeShareable( new FEventGraphData() ) )
	, EventGraphDataMaximum( MakeShareable( new FEventGraphData() ) )
	, EventGraphDataCurrent( MakeShareable( new FEventGraphData() ) )

	, SessionType( EProfilerSessionTypes::Live )
	, SessionInstanceInfo( InSessionInstanceInfo )
	, SessionInstanceID( InSessionInstanceInfo->GetInstanceId() )
	, DataFilepath( TEXT("") )

	, bDataPreviewing( false )
	, bDataCapturing( false )
	, bHasAllProfilerData( false )

	, FPSAnalyzer( MakeShareable( new FFPSAnalyzer(5, 0, 60) ) )
{
	// Randomize creation time to test loading profiler captures with different creation time and different amount of data.
	CreationTime = FDateTime::Now() += FTimespan( 0, 0, FMath::RandRange( 2, 8 ) );

	FTicker::GetCoreTicker().AddTicker( OnTick );
}

FProfilerSession::FProfilerSession( const FString& InDataFilepath )
	: ClientStatMetadata( nullptr )
	, bRequestStatMetadataUpdate( false )
	, bLastPacket( false )
	, StatMetaDataSize( 0 )
	, DataProvider( MakeShareable( new FArrayDataProvider() ) )
	, StatMetaData( MakeShareable( new FProfilerStatMetaData() ) )

	, OnTick( FTickerDelegate::CreateRaw(this, &FProfilerSession::HandleTicker) )

	, EventGraphDataTotal( MakeShareable( new FEventGraphData() ) )
	, EventGraphDataMaximum( MakeShareable( new FEventGraphData() ) )
	, EventGraphDataCurrent( MakeShareable( new FEventGraphData() ) )

	, SessionType( EProfilerSessionTypes::Offline )
	, SessionInstanceID( FGuid::NewGuid() )
	, DataFilepath( InDataFilepath.Replace( TEXT(".ue4stats"), TEXT("") ) )

	, bDataPreviewing( true )
	, bDataCapturing( false )
	, bHasAllProfilerData( false )

	, FPSAnalyzer( MakeShareable( new FFPSAnalyzer(5, 0, 60) ) )
{
	// Randomize creation time to test loading profiler captures with different creation time and different amount of data.
	CreationTime = FDateTime::Now() += FTimespan( 0, 0, FMath::RandRange( 2, 8 ) );

	FTicker::GetCoreTicker().AddTicker( OnTick );
}

FProfilerSession::~FProfilerSession()
{
	FTicker::GetCoreTicker().RemoveTicker( OnTick );	
}

FGraphDataSourceRefConst FProfilerSession::CreateGraphDataSource( const uint32 InStatID )
{
	FGraphDataSource* GraphDataSource = new FGraphDataSource( AsShared(), InStatID );
	return MakeShareable( GraphDataSource );
}

FEventGraphDataRef FProfilerSession::CreateEventGraphData( const uint32 FrameStartIndex, const uint32 FrameEndIndex, const EEventGraphTypes::Type EventGraphType )
{
	static FTotalTimeAndCount Current(0.0f, 0);
	FProfilerScopedLogTime ScopedLog( TEXT("FProfilerSession::CreateEventGraphData"), &Current );

	FEventGraphData* EventGraphData = new FEventGraphData();

	if( EventGraphType == EEventGraphTypes::Average )
	{
		for( uint32 FrameIndex = FrameStartIndex; FrameIndex < FrameEndIndex+1; ++FrameIndex )
		{
			// Create a temporary event graph data for the specified frame.
			const FEventGraphData CurrentEventGraphData( AsShared(), FrameIndex );
			EventGraphData->CombineAndAdd( CurrentEventGraphData );
		}
	
		EventGraphData->Advance( FrameStartIndex, FrameEndIndex+1 );
		EventGraphData->Divide( (double)EventGraphData->GetNumFrames() );
	}
	else if( EventGraphType == EEventGraphTypes::Maximum )
	{
		for( uint32 FrameIndex = FrameStartIndex; FrameIndex < FrameEndIndex+1; ++FrameIndex )
		{
			// Create a temporary event graph data for the specified frame.
			const FEventGraphData CurrentEventGraphData( AsShared(), FrameIndex );
			EventGraphData->CombineAndFindMax( CurrentEventGraphData );
		}
		EventGraphData->Advance( FrameStartIndex, FrameEndIndex+1 );
	}

	return MakeShareable( EventGraphData );
}

void FProfilerSession::UpdateAggregatedStats( const uint32 FrameIndex )
{
	const FIntPoint& IndicesForFrame = DataProvider->GetSamplesIndicesForFrame( FrameIndex );
	const uint32 SampleStartIndex = IndicesForFrame.X;
	const uint32 SampleEndIndex = IndicesForFrame.Y;

	const FProfilerSampleArray& Collection = DataProvider->GetCollection();

	for( uint32 SampleIndex = SampleStartIndex; SampleIndex < SampleEndIndex; SampleIndex++ )
	{
		const FProfilerSample& ProfilerSample = Collection[ SampleIndex ];
		
		const uint32 StatID = ProfilerSample.StatID();
		FProfilerAggregatedStat* AggregatedStat = AggregatedStats.Find( StatID );
		if( !AggregatedStat )
		{
			const FProfilerStat& ProfilerStat = GetMetaData()->GetStat( StatID );
			AggregatedStat = &AggregatedStats.Add( ProfilerSample.StatID(), FProfilerAggregatedStat( ProfilerStat.Name(), ProfilerStat.OwningGroup().Name(), ProfilerSample.Type() ) );
		}
		(*AggregatedStat) += ProfilerSample;
	}

	for( auto It = AggregatedStats.CreateIterator(); It; ++It )
	{
		FProfilerAggregatedStat& AggregatedStat = It.Value();
		AggregatedStat.Advance();
	}

	// @TODO: Create map for stats TMap<uint32, TArray<uint32> >; StatID -> Sample indices for faster lookup in data providers
}

void FProfilerSession::EventGraphCombineAndMax( const FEventGraphDataRef Current, const uint32 NumFrames )
{
	EventGraphDataMaximum->CombineAndFindMax( Current.Get() );
	EventGraphDataMaximum->Advance( 0, NumFrames );
}

void FProfilerSession::EventGraphCombineAndAdd( const FEventGraphDataRef Current, const uint32 NumFrames )
{
	EventGraphDataTotal->CombineAndAdd( Current.Get() );
	EventGraphDataTotal->Advance( 0, NumFrames );
}

void FProfilerSession::UpdateAggregatedEventGraphData( const uint32 FrameIndex )
{
	if( CompletionSync.GetReference() )
	{
		static FTotalTimeAndCount JoinTasksTimeAndCont(0.0f, 0);
		FProfilerScopedLogTime ScopedLog( TEXT("FProfilerSession::CombineJoinAndContinue"), &JoinTasksTimeAndCont );

		FTaskGraphInterface::Get().WaitUntilTaskCompletes(CompletionSync, ENamedThreads::GameThread);
	}

	static FTotalTimeAndCount TimeAndCount(0.0f, 0);
	FProfilerScopedLogTime ScopedLog( TEXT("FProfilerSession::UpdateAggregatedEventGraphData"), &TimeAndCount );

	// Create a temporary event graph data for the specified frame.
	EventGraphDataCurrent = MakeShareable( new FEventGraphData( AsShared(), FrameIndex ) );
	const uint32 NumFrames = DataProvider->GetNumFrames();

	static const bool bUseTaskGraph = true;

	if( bUseTaskGraph )
	{
		FGraphEventArray EventGraphCombineTasks;

		new (EventGraphCombineTasks) FGraphEventRef(FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
		(
			FSimpleDelegateGraphTask::FDelegate::CreateRaw( this, &FProfilerSession::EventGraphCombineAndMax, EventGraphDataCurrent, NumFrames ), 
			TEXT("EventGraphData.CombineAndFindMax"), nullptr
		));

		new (EventGraphCombineTasks) FGraphEventRef(FSimpleDelegateGraphTask::CreateAndDispatchWhenReady
		(
			FSimpleDelegateGraphTask::FDelegate::CreateRaw( this, &FProfilerSession::EventGraphCombineAndAdd, EventGraphDataCurrent, NumFrames ), 
			TEXT("EventGraphData.EventGraphCombineAndAdd"), nullptr
		));

		// JoinThreads
		CompletionSync = TGraphTask<FNullGraphTask>::CreateTask( &EventGraphCombineTasks, ENamedThreads::GameThread )
			.ConstructAndDispatchWhenReady( TEXT("EventGraphData.CombineJoinAndContinue"), ENamedThreads::AnyThread );
	}
	else
	{
		EventGraphCombineAndMax( EventGraphDataCurrent, NumFrames );
		EventGraphCombineAndAdd( EventGraphDataCurrent, NumFrames );
	}
}

bool FProfilerSession::HandleTicker( float DeltaTime )
{
	// Update metadata if needed
	if( bRequestStatMetadataUpdate )
	{
		StatMetaData->Update( *ClientStatMetadata );
		bRequestStatMetadataUpdate = false;
	}

	const int32 FramesNumToProcess = SessionType == EProfilerSessionTypes::Offline ? 240 : 2;

	for( int32 FrameNumber = 0; FrameNumber < FMath::Min(FrameToProcess.Num(),FramesNumToProcess); FrameNumber++ )
	{	
		const uint32 FrameIndex = FrameToProcess[0];
		FrameToProcess.RemoveAt( 0 );

		FProfilerDataFrame& CurrentProfilerData = FrameToProfilerDataMapping.FindChecked( FrameIndex );

		static FTotalTimeAndCount Current(0.0f, 0);
		FProfilerScopedLogTime PSHT( TEXT("FProfilerSession::HandleTicker"), &Current );

		const FProfilerStatMetaDataRef MetaData = GetMetaData();

		// Preprocess the hierarchical samples for the specified frame.
		const TMap<uint32, FProfilerCycleGraph>& CycleGraphs = CurrentProfilerData.CycleGraphs;

		// Add a root sample for this frame.
		// HACK 2013-07-19, 13:44 STAT_Root == ThreadRoot in stats2
		const uint32 FrameRootSampleIndex = DataProvider->AddHierarchicalSample( 0, MetaData->GetStat(1).OwningGroup().ID(), 1, 0.0f, 0.0f, 1 );
		
		double GameThreadTimeMS = 0.0f;
		double MaxThreadTimeMS = 0.0f;

		double ThreadStartTimeMS = 0.0;
		for( auto ThreadIt = CycleGraphs.CreateConstIterator(); ThreadIt; ++ThreadIt )
		{
			const uint32 ThreadID = ThreadIt.Key();
			const FProfilerCycleGraph& ThreadGraph = ThreadIt.Value();

			// Calculate total time for this thread.
			double ThreadDurationTimeMS = 0.0;
			ThreadStartTimeMS = CurrentProfilerData.FrameStart;
			for( int32 Index = 0; Index < ThreadGraph.Children.Num(); Index++ )
			{
				ThreadDurationTimeMS += MetaData->ConvertCyclesToMS( ThreadGraph.Children[Index].Value );
			}

			if (ThreadDurationTimeMS > 0.0)
			{
				// Check for game thread.
				const bool bGameThreadFound = MetaData->ThreadDescriptions.FindChecked( ThreadID ).Contains( TEXT("GameThread") );
				if( bGameThreadFound )
				{
					GameThreadTimeMS = ThreadDurationTimeMS;
				}

				// Add a root sample for each thread.
				const uint32& NewThreadID = MetaData->ThreadIDtoStatID.FindChecked( ThreadID );
				const uint32 ThreadRootSampleIndex = DataProvider->AddHierarchicalSample
				( 
					NewThreadID/*ThreadID*/, 
					MetaData->GetStat(NewThreadID).OwningGroup().ID(), 
					NewThreadID, 
					ThreadStartTimeMS, 
					ThreadDurationTimeMS, 
					1, 
					FrameRootSampleIndex 
				);

				// Recursively add children and parent to the root samples.
				for( int32 Index = 0; Index < ThreadGraph.Children.Num(); Index++ )
				{
					const FProfilerCycleGraph& CycleGraph = ThreadGraph.Children[Index];
					const double CycleDurationMS = MetaData->ConvertCyclesToMS( CycleGraph.Value );
					const double CycleStartTimeMS = ThreadStartTimeMS;

					if (CycleDurationMS > 0.0)
					{
						PopulateHierarchy_Recurrent( CycleGraph, CycleStartTimeMS, CycleDurationMS, ThreadRootSampleIndex );
					}
					ThreadStartTimeMS += CycleDurationMS;
				}

				MaxThreadTimeMS = FMath::Max( MaxThreadTimeMS, ThreadDurationTimeMS );
			}
		}

		// Fix the root stat time.
		FProfilerSampleArray& MutableCollection = const_cast<FProfilerSampleArray&>(DataProvider->GetCollection());
		MutableCollection[FrameRootSampleIndex].SetDurationMS( GameThreadTimeMS != 0.0f ? GameThreadTimeMS : MaxThreadTimeMS );

		// Process the non-hierarchical samples for the specified frame.
		{
			// Process integer counters.
			for( int32 Index = 0; Index < CurrentProfilerData.CountAccumulators.Num(); Index++ )
			{
				const FProfilerCountAccumulator& IntCounter = CurrentProfilerData.CountAccumulators[Index];
				const EProfilerSampleTypes::Type ProfilerSampleType = MetaData->GetSampleTypeForStatID( IntCounter.StatId );
				DataProvider->AddCounterSample( MetaData->GetStat(IntCounter.StatId).OwningGroup().ID(), IntCounter.StatId, (double)IntCounter.Value, ProfilerSampleType );
			}

			// Process floating point counters.
			for( int32 Index = 0; Index < CurrentProfilerData.FloatAccumulators.Num(); Index++ )
			{
				const FProfilerFloatAccumulator& FloatCounter = CurrentProfilerData.FloatAccumulators[Index];
				DataProvider->AddCounterSample( MetaData->GetStat(FloatCounter.StatId).OwningGroup().ID(), FloatCounter.StatId, (double)FloatCounter.Value, EProfilerSampleTypes::NumberFloat );

			}
		}

		// Advance frame
		const uint32 LastFrameIndex = DataProvider->GetNumFrames();
		DataProvider->AdvanceFrame( MaxThreadTimeMS );

		// Update aggregated stats
		UpdateAggregatedStats( LastFrameIndex );

		// Update aggregated events.
		UpdateAggregatedEventGraphData( LastFrameIndex );

		// Cache stat values.
		// @TODO

		FrameToProfilerDataMapping.Remove( FrameIndex );
	}	

	if( SessionType == EProfilerSessionTypes::Offline )
	{
		if( FrameToProcess.Num() == 0 && bHasAllProfilerData )
		{
			// Broadcast that a capture file has been fully processed.
			OnCaptureFileProcessed.ExecuteIfBound( GetInstanceID() );

			// Disable tick method as we no longer need to tick.
			return false;
		}
	}

	return true;
}

void FProfilerSession::PopulateHierarchy_Recurrent
( 
	const FProfilerCycleGraph& ParentGraph, 
	const double ParentStartTimeMS, 
	const double ParentDurationMS,
	const uint32 ParentSampleIndex 
)
{
	const FProfilerStatMetaDataRef MetaData = GetMetaData();
	const uint32& ThreadID = MetaData->ThreadIDtoStatID.FindChecked( ParentGraph.ThreadId );

	const uint32 SampleIndex = DataProvider->AddHierarchicalSample
	( 
		ThreadID, 
		MetaData->GetStat(ParentGraph.StatId).OwningGroup().ID(), 
		ParentGraph.StatId, 
		ParentStartTimeMS, ParentDurationMS, 
		ParentGraph.CallsPerFrame, 
		ParentSampleIndex 
	);

	double ChildStartTimeMS = ParentStartTimeMS;
	double ChildrenDurationMS = 0.0f;

	for( int32 DataIndex = 0; DataIndex < ParentGraph.Children.Num(); DataIndex++ )
	{
		const FProfilerCycleGraph& ChildCyclesCounter = ParentGraph.Children[DataIndex];
		const double ChildDurationMS = MetaData->ConvertCyclesToMS( ChildCyclesCounter.Value );

		if (ChildDurationMS > 0.0)
		{
			PopulateHierarchy_Recurrent( ChildCyclesCounter, ChildStartTimeMS, ChildDurationMS, SampleIndex );
		}
		ChildStartTimeMS += ChildDurationMS;
		ChildrenDurationMS += ChildDurationMS;
	}

	const double SelfTimeMS = ParentDurationMS - ChildrenDurationMS;
	if( SelfTimeMS > 0.0f && ParentGraph.Children.Num() > 0 )
	{
		const FString& ThreadDesc = MetaData->GetThreadDescriptions().FindChecked( ParentGraph.ThreadId );
		const FName& ParentStatName = MetaData->GetStat( ParentGraph.StatId ).Name();
		const FName& ParentGroupName = MetaData->GetStat( ParentGraph.StatId ).OwningGroup().Name();

		// Create a fake stat that represents this profiler sample's exclusive time.
		// This is required if we want to create correct combined event graphs later.
		DataProvider->AddHierarchicalSample
		(
			ThreadID,
			MetaData->GetStat(0).OwningGroup().ID(),
			0, // @see FProfilerStatMetaData.Update, 0 means "Self"
			ChildStartTimeMS, 
			SelfTimeMS,
			1,
			SampleIndex
		);
	}
}
