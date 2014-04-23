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

FProfilerSession::FProfilerSession( EProfilerSessionTypes::Type InSessionType, const ISessionInstanceInfoPtr InSessionInstanceInfo, FGuid InSessionInstanceID, FString InDataFilepath )
: ClientStatMetadata( nullptr )
, bRequestStatMetadataUpdate( false )
, bLastPacket( false )
, StatMetaDataSize( 0 )
, OnTick( FTickerDelegate::CreateRaw( this, &FProfilerSession::HandleTicker ) )
, DataProvider( MakeShareable( new FArrayDataProvider() ) )
, StatMetaData( MakeShareable( new FProfilerStatMetaData() ) )
, EventGraphDataTotal( MakeShareable( new FEventGraphData() ) )
, EventGraphDataMaximum( MakeShareable( new FEventGraphData() ) )
, EventGraphDataCurrent( MakeShareable( new FEventGraphData() ) )
, CreationTime( FDateTime::Now() )
, DataFilepath( InDataFilepath )
, SessionType( InSessionType )
, SessionInstanceInfo( InSessionInstanceInfo )
, SessionInstanceID( InSessionInstanceID )
, bDataPreviewing( false )
, bDataCapturing( false )
, bHasAllProfilerData( false )

, FPSAnalyzer( MakeShareable( new FFPSAnalyzer( 5, 0, 60 ) ) )
{}

FProfilerSession::FProfilerSession( const ISessionInstanceInfoPtr InSessionInstanceInfo )
: ClientStatMetadata( nullptr )
, bRequestStatMetadataUpdate( false )
, bLastPacket( false )
, StatMetaDataSize( 0 )
, OnTick( FTickerDelegate::CreateRaw( this, &FProfilerSession::HandleTicker ) )
, DataProvider( MakeShareable( new FArrayDataProvider() ) )
, StatMetaData( MakeShareable( new FProfilerStatMetaData() ) )
, EventGraphDataTotal( MakeShareable( new FEventGraphData() ) )
, EventGraphDataMaximum( MakeShareable( new FEventGraphData() ) )
, EventGraphDataCurrent( MakeShareable( new FEventGraphData() ) )
, CreationTime( FDateTime::Now() )
, DataFilepath( TEXT("") )
, SessionType( EProfilerSessionTypes::Live )
, SessionInstanceInfo( InSessionInstanceInfo )
, SessionInstanceID( InSessionInstanceInfo->GetInstanceId() )
, bDataPreviewing( false )
, bDataCapturing( false )
, bHasAllProfilerData( false )

, FPSAnalyzer( MakeShareable( new FFPSAnalyzer( 5, 0, 60 ) ) )
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
, OnTick( FTickerDelegate::CreateRaw( this, &FProfilerSession::HandleTicker ) )
, DataProvider( MakeShareable( new FArrayDataProvider() ) )
, StatMetaData( MakeShareable( new FProfilerStatMetaData() ) )
, EventGraphDataTotal( MakeShareable( new FEventGraphData() ) )
, EventGraphDataMaximum( MakeShareable( new FEventGraphData() ) )
, EventGraphDataCurrent( MakeShareable( new FEventGraphData() ) )
, CreationTime( FDateTime::Now() )
, DataFilepath( InDataFilepath.Replace( *FStatConstants::StatsFileExtension, TEXT( "" ) ) )
, SessionType( EProfilerSessionTypes::Offline )
, SessionInstanceInfo( nullptr )
, SessionInstanceID( FGuid::NewGuid() )
, bDataPreviewing( false )
, bDataCapturing( false )
, bHasAllProfilerData( false )

, FPSAnalyzer( MakeShareable( new FFPSAnalyzer( 5, 0, 60 ) ) )
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
	PROFILER_SCOPE_LOG_TIME( TEXT( "FProfilerSession::CreateEventGraphData" ), &Current );

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
			const FProfilerStat& ProfilerStat = GetMetaData()->GetStatByID( StatID );
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
	if( CompletionSync.GetReference() && !CompletionSync->IsComplete() )
	{
		static FTotalTimeAndCount JoinTasksTimeAndCont(0.0f, 0);
		PROFILER_SCOPE_LOG_TIME( TEXT( "FProfilerSession::CombineJoinAndContinue" ), &JoinTasksTimeAndCont );

		FTaskGraphInterface::Get().WaitUntilTaskCompletes(CompletionSync, ENamedThreads::GameThread);
	}

	static FTotalTimeAndCount TimeAndCount(0.0f, 0);
	PROFILER_SCOPE_LOG_TIME( TEXT( "FProfilerSession::UpdateAggregatedEventGraphData" ), &TimeAndCount );

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
		PROFILER_SCOPE_LOG_TIME( TEXT( "FProfilerSession::HandleTicker" ), &Current );

		const FProfilerStatMetaDataRef MetaData = GetMetaData();
		TMap<uint32, float> ThreadMS;

		// Preprocess the hierarchical samples for the specified frame.
		const TMap<uint32, FProfilerCycleGraph>& CycleGraphs = CurrentProfilerData.CycleGraphs;

		// Add a root sample for this frame.
		// HACK 2013-07-19, 13:44 STAT_Root == ThreadRoot in stats2
		const uint32 FrameRootSampleIndex = DataProvider->AddHierarchicalSample( 0, MetaData->GetStatByID(1).OwningGroup().ID(), 1, 0.0f, 0.0f, 1 );
		
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
				const FString GameThreadName = FName( NAME_GameThread ).GetPlainNameString();
				const bool bGameThreadFound = MetaData->GetThreadDescriptions().FindChecked( ThreadID ).Contains( GameThreadName );
				if( bGameThreadFound )
				{
					GameThreadTimeMS = ThreadDurationTimeMS;
				}

				// Add a root sample for each thread.
				const uint32& NewThreadID = MetaData->ThreadIDtoStatID.FindChecked( ThreadID );
				const uint32 ThreadRootSampleIndex = DataProvider->AddHierarchicalSample
				( 
					NewThreadID/*ThreadID*/, 
					MetaData->GetStatByID(NewThreadID).OwningGroup().ID(), 
					NewThreadID, 
					ThreadStartTimeMS, 
					ThreadDurationTimeMS, 
					1, 
					FrameRootSampleIndex 
				);
				ThreadMS.FindOrAdd( ThreadID ) = (float)ThreadDurationTimeMS;

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
				DataProvider->AddCounterSample( MetaData->GetStatByID(IntCounter.StatId).OwningGroup().ID(), IntCounter.StatId, (double)IntCounter.Value, ProfilerSampleType );
			}

			// Process floating point counters.
			for( int32 Index = 0; Index < CurrentProfilerData.FloatAccumulators.Num(); Index++ )
			{
				const FProfilerFloatAccumulator& FloatCounter = CurrentProfilerData.FloatAccumulators[Index];
				DataProvider->AddCounterSample( MetaData->GetStatByID(FloatCounter.StatId).OwningGroup().ID(), FloatCounter.StatId, (double)FloatCounter.Value, EProfilerSampleTypes::NumberFloat );

			}
		}

		// Advance frame
		const uint32 LastFrameIndex = DataProvider->GetNumFrames();
		DataProvider->AdvanceFrame( MaxThreadTimeMS );

		// Update aggregated stats
		UpdateAggregatedStats( LastFrameIndex );

		// Update aggregated events.
		UpdateAggregatedEventGraphData( LastFrameIndex );

		// Update mini-view.
		OnAddThreadTime.ExecuteIfBound( LastFrameIndex, ThreadMS, StatMetaData );

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
		MetaData->GetStatByID(ParentGraph.StatId).OwningGroup().ID(), 
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
		const FName& ParentStatName = MetaData->GetStatByID( ParentGraph.StatId ).Name();
		const FName& ParentGroupName = MetaData->GetStatByID( ParentGraph.StatId ).OwningGroup().Name();

		// Create a fake stat that represents this profiler sample's exclusive time.
		// This is required if we want to create correct combined event graphs later.
		DataProvider->AddHierarchicalSample
		(
			ThreadID,
			MetaData->GetStatByID(0).OwningGroup().ID(),
			0, // @see FProfilerStatMetaData.Update, 0 means "Self"
			ChildStartTimeMS, 
			SelfTimeMS,
			1,
			SampleIndex
		);
	}
}

/*-----------------------------------------------------------------------------
	FRawProfilerSession
-----------------------------------------------------------------------------*/

FRawProfilerSession::FRawProfilerSession( const FString& InRawStatsFileFileath )
: FProfilerSession( EProfilerSessionTypes::OfflineRaw, nullptr, FGuid::NewGuid(), InRawStatsFileFileath.Replace( *FStatConstants::StatsFileRawExtension, TEXT( "" ) ) )
, CurrentMiniViewFrame( 0 )
{
	OnTick = FTickerDelegate::CreateRaw( this, &FRawProfilerSession::HandleTicker );
}

FRawProfilerSession::~FRawProfilerSession()
{
	FTicker::GetCoreTicker().RemoveTicker( OnTick );
}

bool FRawProfilerSession::HandleTicker( float DeltaTime )
{
	StatsThreadStats;
	Stream;

	enum
	{
		MAX_NUM_DATA_PER_TICK = 30
	};
	int32 NumDataThisTick = 0;

	// Add the data to the mini-view.
	for( int32 FrameIndex = CurrentMiniViewFrame; FrameIndex < Stream.FramesInfo.Num(); ++FrameIndex )
	{
		const FStatsFrameInfo& StatsFrameInfo = Stream.FramesInfo[FrameIndex];
		// Convert from cycles to ms.
		TMap<uint32, float> ThreadMS;
		for( auto InnerIt = StatsFrameInfo.ThreadCycles.CreateConstIterator(); InnerIt; ++InnerIt )
		{
			ThreadMS.Add( InnerIt.Key(), StatMetaData->ConvertCyclesToMS( InnerIt.Value() ) );
		}

		// Pass the reference to the stats' metadata.
		// @TODO yrx 2014-04-03 Figure out something better later.
		OnAddThreadTime.ExecuteIfBound( FrameIndex, ThreadMS, StatMetaData );

		//CurrentMiniViewFrame++;
		NumDataThisTick++;
		if( NumDataThisTick > MAX_NUM_DATA_PER_TICK )
		{
			break;
		}
	}

	return true;
}

void FRawProfilerSession::PrepareLoading()
{
	const FString Filepath = DataFilepath + FStatConstants::StatsFileRawExtension;
	const int64 Size = IFileManager::Get().FileSize( *Filepath );
	if( Size < 4 )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filepath );
		return;
	}
	TAutoPtr<FArchive> FileReader( IFileManager::Get().CreateFileReader( *Filepath ) );
	if( !FileReader )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filepath );
		return;
	}

	if( !Stream.ReadHeader( *FileReader ) )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open, bad magic: %s" ), *Filepath );
		return;
	}

	const bool bIsFinalized = Stream.Header.IsFinalized();
	check( bIsFinalized );
	StatsThreadStats.MarkAsLoaded();

	TArray<FStatMessage> Messages;
	if( Stream.Header.bRawStatFile )
	{
		// Read metadata.
		TArray<FStatMessage> MetadataMessages;
		Stream.ReadFNamesAndMetadataMessages( *FileReader, MetadataMessages );
		StatsThreadStats.ProcessMetaDataOnly( MetadataMessages );

		const int64 CurrentFilePos = FileReader->Tell();

		// Update profiler's metadata.
		StatMetaData->UpdateFromStatsState( StatsThreadStats );

		// Read frames offsets.
		Stream.ReadFramesOffsets( *FileReader );

		// Verify frames offsets.
		for( int32 FrameIndex = 0; FrameIndex < Stream.FramesInfo.Num(); ++FrameIndex )
		{
			const FStatsFrameInfo& StatsFrameInfo = Stream.FramesInfo[FrameIndex];
			FileReader->Seek( StatsFrameInfo.FrameOffset );
			int64 TargetFrame;
			*FileReader << TargetFrame;
			TargetFrame = TargetFrame;
		}
		int64 FrameOffset0 = Stream.FramesInfo[0].FrameOffset;
		FileReader->Seek( FrameOffset0 );
#if	0
		// Read the raw stats messages.
		for( int32 FrameIndex = 0; FrameIndex < 30/*Stream.FramesInfo.Num()*/; ++FrameIndex )
		{
			FStatPacketArray Packet;

			int64 TargetFrame;
			*FileReader << TargetFrame;

			int32 NumPackets;
			*FileReader << NumPackets;

			for( int32 PacketIndex = 0; PacketIndex < NumPackets; PacketIndex++ )
			{
				FStatPacket* ToRead = new FStatPacket();
				Stream.ReadStatPacket( *FileReader, *ToRead, bIsFinalized );
				Packet.Packets.Add( ToRead );
			}

			ProcessStatPacketArray( Packet );

			TMap<uint32, int64> ThreadCycles;
			// Serialize thread cycles.
			*FileReader << ThreadCycles;
		}
#endif // 0
	}

	// We have the whole metadata and basic information about the raw stats file, start ticking the profiler session.
	FTicker::GetCoreTicker().AddTicker( OnTick, 0.25f );

#if	0
	if( SessionType == EProfilerSessionTypes::OfflineRaw )
	{
		// Broadcast that a capture file has been fully processed.
		OnCaptureFileProcessed.ExecuteIfBound( GetInstanceID() );
	}
#endif // 0
}

void FProfilerStatMetaData::UpdateFromStatsState( const FStatsThreadState& StatsThreadStats )
{
	TMap<FName, int32> GroupFNameIDs;

	StatsThreadStats.Groups;
	StatsThreadStats.MemoryPoolToCapacityLongName;
	StatsThreadStats.NotClearedEveryFrame;
	StatsThreadStats.Threads;
	StatsThreadStats.ShortNameToLongName;

	for( auto It = StatsThreadStats.Threads.CreateConstIterator(); It; ++It )
	{
		ThreadDescriptions.Add( It.Key(), It.Value().ToString() );
	}

	const uint32 NoGroupID = 0;
	const uint32 ThreadGroupID = 1;

	// Special groups.
	InitializeGroup( NoGroupID, "NoGroup" );

	// Self must be 0.
	InitializeStat( 0, NoGroupID, TEXT( "Self" ), STATTYPE_CycleCounter );

	// ThreadRoot must be 1.
	InitializeStat( 1, NoGroupID, FStatConstants::NAME_ThreadRoot.GetPlainNameString(), STATTYPE_CycleCounter, FStatConstants::NAME_ThreadRoot );

	int32 UniqueID = 15;

	TArray<FName> GroupFNames;
	StatsThreadStats.Groups.MultiFind( NAME_Groups, GroupFNames );
	for( const auto& GroupFName : GroupFNames  )
	{
		UniqueID++;
		InitializeGroup( UniqueID, GroupFName.ToString() );
		GroupFNameIDs.Add( GroupFName, UniqueID );
	}

	for( auto It = StatsThreadStats.ShortNameToLongName.CreateConstIterator(); It; ++It )
	{
		const FStatMessage& LongName = It.Value();
		
		const FName GroupName = LongName.NameAndInfo.GetGroupName();
		if( GroupName == NAME_Groups )
		{
			continue;
		}
		const int32 GroupID = GroupFNameIDs.FindChecked( GroupName );

		FString StatDesc;
		LongName.NameAndInfo.GetDescription( StatDesc );

		const FName StatName = It.Key();
		UniqueID++;

		EStatType StatType = STATTYPE_Error;
		if( LongName.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64 )
		{
			if( LongName.NameAndInfo.GetFlag( EStatMetaFlags::IsCycle ) )
			{
				StatType = STATTYPE_CycleCounter;
			}
			else if( LongName.NameAndInfo.GetFlag( EStatMetaFlags::IsMemory ) )
			{
				StatType = STATTYPE_MemoryCounter;
			}
			else
			{
				StatType = STATTYPE_AccumulatorDWORD;
			}
		}
		else if( LongName.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double )
		{
			StatType = STATTYPE_AccumulatorFLOAT;
		}

		check( StatType != STATTYPE_Error );

		int32 StatID = UniqueID;
		// Some hackery.
		if( StatName == TEXT( "STAT_FrameTime" ) )
		{
			StatID = 2;
		}

		if( StatDesc.Len() == 0 )
		{
			StatDesc = StatName.ToString();
		}

		InitializeStat( StatID, GroupID, StatDesc, StatType, StatName );

		// Setup thread id to stat id.
		if( GroupName == FStatConstants::NAME_ThreadGroup )
		{
			uint32 ThreadID = 0;
			for( auto It = StatsThreadStats.Threads.CreateConstIterator(); It; ++It )
			{
				if( It.Value() == StatName )
				{
					ThreadID = It.Key();
				}
			}
			ThreadIDtoStatID.Add( ThreadID, StatID );

			if( StatName == NAME_GameThread )
			{
				GameThreadID = ThreadID;
			}
			else if( StatName == NAME_RenderThread )
			{
				RenderThreadIDs.Add( ThreadID );
			}
		}
	}

	// @TODO yrx 2014-03-24 Fix this.
	SecondsPerCycle = FPlatformTime::GetSecondsPerCycle();
}

/** A simple stats stack node, used as a proxy class to convert data from the stats into the profiler. */
struct FStatStackNode
{
	/** Short name. */
	FName NodeName;
	int64 CyclesStart;
	int64 CyclesEnd;
	TArray<FStatStackNode*> Children;

	/** Index of this node in the data provider's collection. */
	uint32 SampleIndex;

	explicit FStatStackNode( const FStatMessage& StatMessage, uint32 InSampleIndex )
		: NodeName( StatMessage.NameAndInfo.GetShortName() )
		, CyclesStart( StatMessage.GetValue_int64() )
		, CyclesEnd( 0 )
		, SampleIndex( InSampleIndex )
	{}

	~FStatStackNode()
	{
		for( const auto& Child : Children )
		{
			delete Child;
		}
	}
};

void FRawProfilerSession::ProcessStatPacketArray( const FStatPacketArray& StatPacketArray )
{
	// @TODO yrx 2014-03-24 Standardize thread names and id

	// Raw stats callstack for this stat packet array.
	TMap<FName,FStatStackNode> ThreadNodes;

	const FProfilerStatMetaDataRef MetaData = GetMetaData();
	
	FProfilerSampleArray& MutableCollection = const_cast<FProfilerSampleArray&>(DataProvider->GetCollection());

	// Add a root sample for this frame.
	const uint32 FrameRootSampleIndex = DataProvider->AddHierarchicalSample( 0, MetaData->GetStatByID( 1 ).OwningGroup().ID(), 1, 0.0f, 0.0f, 1 );

	// Iterate through all stats packets and raw stats messages.
	FName GameThreadFName = NAME_None;
	for( int32 PacketIndex = 0; PacketIndex < StatPacketArray.Packets.Num(); PacketIndex++ )
	{
		const FStatPacket& StatPacket = *StatPacketArray.Packets[PacketIndex];
		const FName ThreadFName = StatsThreadStats.Threads.FindChecked( StatPacket.ThreadId );
		const uint32 NewThreadID = MetaData->ThreadIDtoStatID.FindChecked( StatPacket.ThreadId );

		if( StatPacket.ThreadType != EThreadType::Game )
		{
			continue;
		}

		FStatStackNode* ThreadNode = ThreadNodes.Find( ThreadFName );
		if( !ThreadNode )
		{
			FStatMessage ThreadMessage( ThreadFName, EStatDataType::ST_int64, nullptr, TEXT( "" ), true, true );
			ThreadMessage.NameAndInfo.SetFlag( EStatMetaFlags::IsPackedCCAndDuration, true );
			ThreadMessage.Clear();

			// Add a thread sample.
			const uint32 ThreadRootSampleIndex = DataProvider->AddHierarchicalSample
			(
				NewThreadID,
				MetaData->GetStatByID( NewThreadID ).OwningGroup().ID(),
				NewThreadID,
				-1.0f,
				-1.0f,
				1,
				FrameRootSampleIndex
			);

			ThreadNode = &ThreadNodes.Add( ThreadFName, FStatStackNode( ThreadMessage, ThreadRootSampleIndex ) );
		}

		if( StatPacket.ThreadType == EThreadType::Game )
		{
			GameThreadFName = ThreadFName;
		}

		TArray<const FStatMessage*> StartStack;
		TArray<FStatStackNode*> Stack;
		Stack.Add( ThreadNode );
		FStatStackNode* Current = Stack.Last();

		const TArray<FStatMessage>& Data = StatPacket.StatMessages;
		for( int32 Index = 0; Index < Data.Num(); Index++ )
		{
			const FStatMessage& Item = Data[Index];
			check( Item.NameAndInfo.GetFlag( EStatMetaFlags::DummyAlwaysOne ) );  // we should never be sending short names to the stats anymore

			const EStatOperation::Type Op = Item.NameAndInfo.GetField<EStatOperation>();
			const FName LongName = Item.NameAndInfo.GetRawName();
			const FName ShortName = Item.NameAndInfo.GetShortName();

			if( Op == EStatOperation::CycleScopeStart || Op == EStatOperation::CycleScopeEnd )
			{
				check( Item.NameAndInfo.GetFlag( EStatMetaFlags::IsCycle ) );
				if( Op == EStatOperation::CycleScopeStart )
				{
					FStatStackNode* ChildNode = new FStatStackNode( Item, -1 );
					Current->Children.Add( ChildNode );

					// Add a child sample.
					const uint32 SampleIndex = DataProvider->AddHierarchicalSample
					(
						NewThreadID,
						MetaData->GetStatByFName( ShortName ).OwningGroup().ID(), // GroupID
						MetaData->GetStatByFName( ShortName ).ID(), // StatID
						MetaData->ConvertCyclesToMS( ChildNode->CyclesStart ), // StartMS 
						MetaData->ConvertCyclesToMS( 0 ), // DurationMS
						1,
						Current->SampleIndex
					);
					ChildNode->SampleIndex = SampleIndex;

					Stack.Add( ChildNode );
					StartStack.Add( &Item );
					Current = ChildNode;
				}
				if( Op == EStatOperation::CycleScopeEnd )
				{
					const FStatMessage ScopeStart = *StartStack.Pop();
					const FStatMessage ScopeEnd = Item;
					const int64 Delta = int32( uint32( ScopeEnd.GetValue_int64() ) - uint32( ScopeStart.GetValue_int64() ) );
					Current->CyclesEnd = Current->CyclesStart + int64( Delta );
					FStatStackNode* ChildNode = Current;

					// Update the child sample's DurationMS.
					MutableCollection[ChildNode->SampleIndex].SetDurationMS( MetaData->ConvertCyclesToMS( Delta ) );

					verify( Current == Stack.Pop() );
					Current = Stack.Last();				
				}
			}
		}
	}

	// Calculate thread times.
	for( auto It = ThreadNodes.CreateIterator(); It; ++It )
	{
		FStatStackNode& ThreadNode = It.Value();
		const int32 ChildrenNum = ThreadNode.Children.Num();
		if( ChildrenNum > 0 )
		{
			const int32 LastChildIndex = ThreadNode.Children.Num() - 1;
			ThreadNode.CyclesStart = ThreadNode.Children[0]->CyclesStart;
			ThreadNode.CyclesEnd = ThreadNode.Children[LastChildIndex]->CyclesEnd;

			FProfilerSample& ProfilerSample = MutableCollection[ThreadNode.SampleIndex];
			ProfilerSample.SetStartAndEndMS( MetaData->ConvertCyclesToMS( ThreadNode.CyclesStart ), MetaData->ConvertCyclesToMS( ThreadNode.CyclesEnd ) );
		}
	}

	// Get the game thread time.
	check( GameThreadFName != NAME_None );
	const FStatStackNode& GameThreadNode = ThreadNodes.FindChecked( GameThreadFName );
	const double GameThreadStartMS = MetaData->ConvertCyclesToMS( GameThreadNode.CyclesStart );
	const double GameThreadEndMS = MetaData->ConvertCyclesToMS( GameThreadNode.CyclesEnd );
	MutableCollection[FrameRootSampleIndex].SetStartAndEndMS( GameThreadStartMS, GameThreadEndMS );
	
	// Advance frame
	const uint32 LastFrameIndex = DataProvider->GetNumFrames();
	DataProvider->AdvanceFrame( GameThreadEndMS - GameThreadStartMS );
 
	// Update aggregated stats
	UpdateAggregatedStats( LastFrameIndex );
 
	// Update aggregated events.
	UpdateAggregatedEventGraphData( LastFrameIndex );
}