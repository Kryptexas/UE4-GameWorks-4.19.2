// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ProfilerPrivatePCH.h"
#include "RenderingThread.h"

#define LOCTEXT_NAMESPACE "FProfilerCommands"

/*-----------------------------------------------------------------------------
	Globals and statics
-----------------------------------------------------------------------------*/
DEFINE_LOG_CATEGORY(Profiler);
FProfilerSettings FProfilerSettings::Defaults( true );

/*-----------------------------------------------------------------------------
	Stat declarations.
-----------------------------------------------------------------------------*/



DEFINE_STAT(STAT_DG_OnPaint);
DEFINE_STAT(STAT_PM_HandleProfilerData);
DEFINE_STAT(STAT_PM_Tick);
DEFINE_STAT(STAT_PM_MemoryUsage);

/*-----------------------------------------------------------------------------
	Profiler manager implementation
-----------------------------------------------------------------------------*/

TSharedPtr<FProfilerManager> FProfilerManager::Instance = nullptr;

struct FEventGraphSampleLess
{
	FORCEINLINE bool operator()(const FEventGraphSample& A, const FEventGraphSample& B) const 
	{ 
		return A._InclusiveTimeMS < B._InclusiveTimeMS; 
	}
};

FProfilerManager::FProfilerManager( const ISessionManagerRef& InSessionManager )
	: SessionManager( InSessionManager )
	, CommandList( new FUICommandList() )
	, ProfilerActionManager( this )
	, Settings()

	, ProfilerType( EProfilerSessionTypes::InvalidOrMax )
	, bLivePreview( false )
	, bHasCaptureFileFullyProcessed( false )
{
	FEventGraphSample::InitializePropertyManagement();

#if	0
	// Performance tests.
	static FTotalTimeAndCount CurrentNative(0.0f, 0);
	static FTotalTimeAndCount CurrentPointer(0.0f, 0);
	static FTotalTimeAndCount CurrentShared(0.0f, 0);

	for( int32 Lx = 0; Lx < 16; ++Lx )
	{
		FRandomStream RandomStream( 666 );
		TArray<FEventGraphSample> EventsNative;
		TArray<FEventGraphSample*> EventsPointer;
		TArray<FEventGraphSamplePtr> EventsShared;

		const int32 NumEvents = 16384;
		for( int32 Nx = 0; Nx < NumEvents; ++Nx )
		{
			const double Rnd = RandomStream.FRandRange( 0.0f, 16384.0f );
			const FString EventName = TTypeToString<double>::ToString( Rnd );
			FEventGraphSample NativeEvent( *EventName );
			NativeEvent._InclusiveTimeMS = Rnd;

			FEventGraphSamplePtr SharedEvent = FEventGraphSample::CreateNamedEvent( *EventName );
			SharedEvent->_InclusiveTimeMS = Rnd;

			EventsNative.Add(NativeEvent);
			EventsPointer.Add(SharedEvent.Get());
			EventsShared.Add(SharedEvent);
		}

		{
			FProfilerScopedLogTime ScopedLog( TEXT("1.NativeSorting"), &CurrentNative );
			EventsNative.Sort( FEventGraphSampleLess() );
		}

		{
			FProfilerScopedLogTime ScopedLog( TEXT("2PointerSorting"), &CurrentPointer );
			EventsPointer.Sort( FEventGraphSampleLess() );
		}

		{
			FProfilerScopedLogTime ScopedLog( TEXT("3.SharedSorting"), &CurrentShared );
			FEventArraySorter::Sort( EventsShared, FEventGraphSample::GetEventPropertyByIndex(EEventPropertyIndex::InclusiveTimeMS).Name, EEventCompareOps::Less );
		}
	}
#endif // 0
}

void FProfilerManager::PostConstructor()
{
	// Register tick functions.
	OnTick = FTickerDelegate::CreateSP( this, &FProfilerManager::Tick );
	OnTickHandle = FTicker::GetCoreTicker().AddTicker( OnTick );

	// Create profiler client.
	ProfilerClient = FModuleManager::GetModuleChecked<IProfilerClientModule>("ProfilerClient").CreateProfilerClient();
	
	// Register profiler client delegates.
	ProfilerClient->OnProfilerData().AddSP(this, &FProfilerManager::ProfilerClient_OnProfilerData);
	ProfilerClient->OnProfilerClientConnected().AddSP(this, &FProfilerManager::ProfilerClient_OnClientConnected);
	ProfilerClient->OnProfilerClientDisconnected().AddSP(this, &FProfilerManager::ProfilerClient_OnClientDisconnected);
	
	ProfilerClient->OnLoadStarted().AddSP(this, &FProfilerManager::ProfilerClient_OnLoadStarted);
	ProfilerClient->OnLoadCompleted().AddSP(this, &FProfilerManager::ProfilerClient_OnLoadCompleted);

	ProfilerClient->OnMetaDataUpdated().AddSP(this, &FProfilerManager::ProfilerClient_OnMetaDataUpdated);

	ProfilerClient->OnProfilerFileTransfer().AddSP(this, &FProfilerManager::ProfilerClient_OnProfilerFileTransfer);

	SessionManager->OnSelectedSessionChanged().AddSP( this, &FProfilerManager::SessionManager_OnSelectedSessionChanged );
	SessionManager->OnInstanceSelectionChanged().AddSP( this, &FProfilerManager::SessionManager_OnInstanceSelectionChanged );

	SetDataPreview( false );
	SetDataCapture( false );

	FProfilerCommands::Register();
	BindCommands();
}

void FProfilerManager::BindCommands()
{
	ProfilerActionManager.Map_ProfilerManager_Load();
	ProfilerActionManager.Map_ToggleDataPreview_Global();
	ProfilerActionManager.Map_ProfilerManager_ToggleLivePreview_Global();
	ProfilerActionManager.Map_ToggleDataCapture_Global();
	ProfilerActionManager.Map_OpenSettings_Global();
}

FProfilerManager::~FProfilerManager()
{
	FProfilerCommands::Unregister();

	// Unregister tick function.
	FTicker::GetCoreTicker().RemoveTicker( OnTickHandle );

	// Remove ourselves from the session manager.
	if (SessionManager.IsValid())
	{
		// clear the selected session
		ISessionInfoPtr Ptr;
		Ptr.Reset();
		SessionManager->SelectSession( Ptr );

		SessionManager->OnCanSelectSession().RemoveAll( this );
		SessionManager->OnSelectedSessionChanged().RemoveAll( this );
		SessionManager->OnInstanceSelectionChanged().RemoveAll( this );
	}

	// Remove ourselves from the profiler client.
	if( ProfilerClient.IsValid() )
	{
		ProfilerClient->Unsubscribe();

		ProfilerClient->OnProfilerData().RemoveAll(this);
		ProfilerClient->OnProfilerClientConnected().RemoveAll(this);
		ProfilerClient->OnProfilerClientDisconnected().RemoveAll(this);
		ProfilerClient->OnMetaDataUpdated().RemoveAll(this);
		ProfilerClient->OnLoadCompleted().RemoveAll(this);
		ProfilerClient->OnLoadStarted().RemoveAll(this);
		ProfilerClient->OnProfilerFileTransfer().RemoveAll(this);
	}

	ClearStatsAndInstances();
}

class FStatsHeaderReader : public FStatsReadFile
{
	friend struct FStatsReader<FStatsHeaderReader>;
	typedef FStatsReadFile Super;

protected:

	/** Initialization constructor. */
	FStatsHeaderReader( const TCHAR* InFilename )
		: FStatsReadFile( InFilename, false )
	{}
};

static int32 GetNumFrameFromCaptureSlow( const FString& ProfilerCaptureFilepath )
{
	TAutoPtr<FStatsHeaderReader> Instance( FStatsReader<FStatsHeaderReader>::Create( *ProfilerCaptureFilepath ) );
	return Instance->GetNumFrames();
}

void FProfilerManager::LoadProfilerCapture( const FString& ProfilerCaptureFilepath )
{
	// Deselect the active session.
	if (ActiveSession.IsValid())
	{
		SessionManager->SelectSession( nullptr );
	}

	ClearStatsAndInstances();
	
	ProfilerSession = MakeShareable( new FProfilerSession( ProfilerCaptureFilepath ) );
	ActiveInstanceID = ProfilerSession->GetInstanceID();

	ProfilerSession->
		SetOnCaptureFileProcessed( FProfilerSession::FCaptureFileProcessedDelegate::CreateSP( this, &FProfilerManager::ProfilerSession_OnCaptureFileProcessed ) )
		.SetOnAddThreadTime( FProfilerSession::FAddThreadTimeDelegate::CreateSP( this, &FProfilerManager::ProfilerSession_OnAddThreadTime ) );

	ProfilerClient->LoadCapture( ProfilerCaptureFilepath, ActiveInstanceID );

	const int32 NumFrames = GetNumFrameFromCaptureSlow( ProfilerCaptureFilepath );
	ProfilerSession->SetNumberOfFrames( NumFrames );

	SessionInstancesUpdatedEvent.Broadcast();
	ProfilerType = EProfilerSessionTypes::StatsFile;
	
	GetProfilerWindow()->ManageEventGraphTab( ActiveInstanceID, true, ProfilerSession->GetName() );
	SetViewMode( EProfilerViewMode::LineIndexBased );
}

void FProfilerManager::LoadRawStatsFile( const FString& RawStatsFileFileath )
{
	// #YRX_Rework 2015-11-30 
#if	0
	if (ActiveSession.IsValid())
	{
		SessionManager->SelectSession( NULL );
	}
	ClearStatsAndInstances();

	TSharedRef<FRawProfilerSession> ProfilerSession = MakeShareable( new FRawProfilerSession( RawStatsFileFileath ) );
	const FGuid ProfilerInstanceID = ProfilerSession->GetInstanceID();
	//ProfilerSessionInstances.Add( ProfilerInstanceID, ProfilerSession );

	ProfilerSession->
		//SetOnCaptureFileProcessed( FProfilerSession::FCaptureFileProcessedDelegate::CreateSP( this, /&FProfilerManager::ProfilerSession_OnCaptureFileProcessed ) )
		SetOnAddThreadTime( FProfilerSession::FAddThreadTimeDelegate::CreateSP( this, &FProfilerManager::ProfilerSession_OnAddThreadTime ) );

	//ProfilerSessionInstances.Add( ProfilerInstanceID, ProfilerSession );

	ProfilerSession->PrepareLoading();
	RequestFilterAndPresetsUpdateEvent.Broadcast();
	//ProfilerSession_OnCaptureFileProcessed( ProfilerInstanceID );
	bHasCaptureFileFullyProcessed = true;
	//TrackDefaultStats();

	SessionInstancesUpdatedEvent.Broadcast();
	ProfilerType = EProfilerSessionTypes::StatsFileRaw;

	GetProfilerWindow()->ManageEventGraphTab( ProfilerInstanceID, true, ProfilerSession->GetName() );
	SetViewMode( EProfilerViewMode::ThreadViewTimeBased );

	GetProfilerWindow()->GraphPanel->ThreadView->AttachProfilerStream( ProfilerSession->GetStream() );
#endif // 0
}


bool FProfilerManager::Tick( float DeltaTime )
{
	SCOPE_CYCLE_COUNTER(STAT_PM_Tick);

#if	0
	static int32 NumIdleTicks = 0;
	NumIdleTicks++;
	if( NumIdleTicks > 60 )
	{
		LoadRawStatsFile( TEXT( "U:/P4EPIC2/UE4/QAGame/Saved/Profiling/UnrealStats/RenderTestMap-Windows-03.19-13.49.04/RenderTestMap-Windows-19-13.52.19.ue4statsraw" ) );
		return false;
	}
#endif // 0

	return true;
}

double LoadStartTime = 0.0;

void FProfilerManager::ProfilerSession_OnCaptureFileProcessed( const FGuid ProfilerInstanceID )
{
	if (ProfilerSession.IsValid() && ProfilerWindow.IsValid())
	{
		TrackDefaultStats();

		RequestFilterAndPresetsUpdateEvent.Broadcast();

		GetProfilerWindow()->UpdateEventGraph( ProfilerInstanceID, ProfilerSession->GetEventGraphDataAverage(), ProfilerSession->GetEventGraphDataMaximum(), true );
		bHasCaptureFileFullyProcessed = true;

		const double TotalLoadTime = FPlatformTime::Seconds() - LoadStartTime;

		const FString Description = ProfilerSession->GetName();
		UE_LOG( LogStats, Warning, TEXT( "OnCaptureFileProcessed: %s" ), *Description );
		UE_LOG( LogStats, Warning, TEXT( "TotalLoadTime: %.2f" ), TotalLoadTime );

		// Update the notification that a file has been fully processed.
		GetProfilerWindow()->ManageLoadingProgressNotificationState( Description, EProfilerNotificationTypes::LoadingOfflineCapture, ELoadingProgressStates::Loaded, 1.0f );
	}
}

const bool FProfilerManager::IsDataPreviewing() const
{
	return IsConnected() && ProfilerSession.IsValid() && ProfilerSession->bDataPreviewing;
}

void FProfilerManager::SetDataPreview( const bool bRequestedDataPreviewState )
{
	ProfilerClient->SetPreviewState( bRequestedDataPreviewState );
	if (ProfilerSession.IsValid())
	{
		ProfilerSession->bDataPreviewing = bRequestedDataPreviewState;
	}
}

const bool FProfilerManager::IsDataCapturing() const
{
	return IsConnected() && ProfilerSession.IsValid() && ProfilerSession->bDataCapturing;
}

void FProfilerManager::SetDataCapture( const bool bRequestedDataCaptureState )
{
	ProfilerClient->SetCaptureState( bRequestedDataCaptureState );
	if (ProfilerSession.IsValid())
	{
		ProfilerSession->bDataCapturing = bRequestedDataCaptureState;
	}
}

/*-----------------------------------------------------------------------------
	ProfilerClient
-----------------------------------------------------------------------------*/

void FProfilerManager::ProfilerClient_OnProfilerData( const FGuid& InstanceID, const FProfilerDataFrame& Content )
{
	SCOPE_CYCLE_COUNTER(STAT_PM_HandleProfilerData);

	if (ProfilerSession.IsValid())
	{
		ProfilerSession->UpdateProfilerData( Content );
		// Game thread should always be enabled.
		TrackDefaultStats();
	}
}

void FProfilerManager::ProfilerClient_OnMetaDataUpdated( const FGuid& InstanceID, const FStatMetaData& MetaData )
{
	if (ProfilerSession.IsValid())
	{
		ProfilerSession->UpdateMetadata( MetaData );

		if (ProfilerSession->GetSessionType() == EProfilerSessionTypes::Live)
		{
			RequestFilterAndPresetsUpdateEvent.Broadcast();
		}
	}
}



void FProfilerManager::ProfilerClient_OnLoadStarted( const FGuid& InstanceID )
{
	if (ProfilerSession.IsValid() && GetProfilerWindow().IsValid())
	{
		const FString Description = ProfilerSession->GetName();
		UE_LOG( LogStats, Warning, TEXT( "OnLoadStarted: %s" ), *Description );
		LoadStartTime = FPlatformTime::Seconds();

		// Display the notification that a file is being loaded.
		GetProfilerWindow()->ManageLoadingProgressNotificationState( Description, EProfilerNotificationTypes::LoadingOfflineCapture, ELoadingProgressStates::Started, 0.0f );
	}
}

void FProfilerManager::ProfilerClient_OnLoadCompleted( const FGuid& InstanceID )
{
	// Inform that the file has been loaded and we can hide the notification.
	if (ProfilerSession.IsValid())
	{
		ProfilerSession->LoadComplete();

		const FString Description = ProfilerSession->GetName();
		UE_LOG( LogStats, Warning, TEXT( "OnLoadCompleted: %s" ), *Description );
	}
}


void FProfilerManager::ProfilerClient_OnProfilerFileTransfer( const FString& Filename, int64 FileProgress, int64 FileSize )
{
	// Display and update the notification a file that is being sent.

	const float Progress = (double)FileProgress/(double)FileSize;

	ELoadingProgressStates::Type ProgressState = ELoadingProgressStates::InvalidOrMax;
	if( FileProgress == 0 )
	{
		ProgressState = ELoadingProgressStates::Started;
	}
	else if( FileProgress == -1 && FileSize == -1 )
	{
		ProgressState = ELoadingProgressStates::Failed;
	}
	else if( FileProgress > 0 && FileProgress < FileSize )
	{
		ProgressState = ELoadingProgressStates::InProgress;
	}
	else if( FileProgress == FileSize )
	{
		ProgressState = ELoadingProgressStates::Loaded;
	}
	
	if( ProfilerWindow.IsValid() )
	{
		ProfilerWindow.Pin()->ManageLoadingProgressNotificationState( Filename, EProfilerNotificationTypes::SendingServiceSideCapture, ProgressState, Progress );
	}
}

void FProfilerManager::ProfilerClient_OnClientConnected( const FGuid& SessionID, const FGuid& InstanceID )
{
}

void FProfilerManager::ProfilerClient_OnClientDisconnected( const FGuid& SessionID, const FGuid& InstanceID )
{
}

/*-----------------------------------------------------------------------------
	SessionManager
-----------------------------------------------------------------------------*/

void FProfilerManager::SessionManager_OnSelectedSessionChanged( const ISessionInfoPtr& InActiveSession )
{
	//SessionManager_OnInstanceSelectionChanged(nullptr, false);
}

void FProfilerManager::SessionManager_OnInstanceSelectionChanged(const TSharedPtr<ISessionInstanceInfo>& InInstance, bool Selected)
{
	const ISessionInfoPtr& SelectedSession = SessionManager->GetSelectedSession();
	const bool SessionIsValid = SelectedSession.IsValid()
		&& (SelectedSession->GetSessionOwner() == FPlatformProcess::UserName(false))
		&& (SessionManager->GetSelectedInstances().Num() > 0);

	if (InInstance->GetInstanceId() != ActiveInstanceID && SessionIsValid && InInstance.IsValid() && Selected)
	{
		ClearStatsAndInstances();

		if (SessionIsValid)
		{
			ActiveSession = SelectedSession;
			ActiveInstanceID = InInstance->GetInstanceId();
			ProfilerClient->Subscribe( ActiveSession->GetSessionId() );
			ProfilerType = EProfilerSessionTypes::Live;
			SetViewMode( EProfilerViewMode::LineIndexBased );
		}
		else
		{
			ActiveSession = nullptr;
			ProfilerClient->Unsubscribe();
			ProfilerType = EProfilerSessionTypes::InvalidOrMax;
		}

		if (ActiveSession.IsValid())
		{
			ProfilerSession = MakeShareable( new FProfilerSession( InInstance ) );
			ProfilerSession->SetOnAddThreadTime( FProfilerSession::FAddThreadTimeDelegate::CreateSP( this, &FProfilerManager::ProfilerSession_OnAddThreadTime ) );
			ProfilerClient->Track( ActiveInstanceID );
			TSharedPtr<SProfilerWindow> ProfilerWindowPtr = GetProfilerWindow();
			if (ProfilerWindowPtr.IsValid())
			{
				ProfilerWindowPtr->ManageEventGraphTab( ActiveInstanceID, true, ProfilerSession->GetName() );
			}			
		}
	}

	SessionInstancesUpdatedEvent.Broadcast();
}

/*-----------------------------------------------------------------------------
	Misc
-----------------------------------------------------------------------------*/

FCombinedGraphDataSourceRef FProfilerManager::CreateCombinedGraphDataSource( const uint32 StatID )
{
	FCombinedGraphDataSource* CombinedGraphDataSource = new FCombinedGraphDataSource( StatID, FTimeAccuracy::FPS060 );
	return MakeShareable( CombinedGraphDataSource );
}

/*-----------------------------------------------------------------------------
	Stat tracking
-----------------------------------------------------------------------------*/

bool FProfilerManager::TrackStat( const uint32 StatID )
{
	bool bAdded = false;

	// Check if all profiler instances have this stat ready.
	const bool bStatIsReady = ProfilerSession->GetAggregatedStat( StatID ) != nullptr;
	if( StatID != 0 && bStatIsReady )
	{
		FTrackedStat* TrackedStat = TrackedStats.Find( StatID );

		if( TrackedStat == nullptr )
		{
			// R = H, G = S, B = V
			const FLinearColor& ColorAverage = GetColorForStatID( StatID );
			const FLinearColor ColorAverageHSV = ColorAverage.LinearRGBToHSV();

			FLinearColor ColorBackgroundHSV = ColorAverageHSV;
			ColorBackgroundHSV.G = FMath::Max( 0.0f, ColorBackgroundHSV.G-0.25f );

			FLinearColor ColorExtremesHSV = ColorAverageHSV;
			ColorExtremesHSV.G = FMath::Min( 1.0f, ColorExtremesHSV.G+0.25f );
			ColorExtremesHSV.B = FMath::Min( 1.0f, ColorExtremesHSV.B+0.25f );

			const FLinearColor ColorBackground = ColorBackgroundHSV.HSVToLinearRGB();
			const FLinearColor ColorExtremes = ColorExtremesHSV.HSVToLinearRGB();

			TrackedStat = &TrackedStats.Add( StatID, FTrackedStat(CreateCombinedGraphDataSource( StatID ),ColorAverage,ColorExtremes,ColorBackground,StatID) );
			bAdded = true;

			// @TODO: Convert a reference parameter to copy parameter/sharedptr/ref/weak, to avoid problems when a reference is no longer valid.
			TrackedStatChangedEvent.Broadcast( *TrackedStat, true );
		}

		if( TrackedStat != nullptr )
		{
			uint32 NumAddedInstances = 0;
			bool bMetadataInitialized = false;

			if (ProfilerSession.IsValid())
			{

				const bool bInstanceAdded = TrackStatForSessionInstance( StatID, ActiveInstanceID );
				NumAddedInstances += bInstanceAdded ? 1 : 0;

				// Initialize metadata for combine graph data source.
				if( !bMetadataInitialized )
				{
					const bool bIsStatReady = ProfilerSession->GetMetaData()->IsStatInitialized( StatID );
					if( bIsStatReady )
					{
						const FProfilerStatMetaDataRef MetaData = ProfilerSession->GetMetaData();
						const FProfilerStat& Stat = MetaData->GetStatByID( StatID );
						const FProfilerGroup& Group = Stat.OwningGroup();

						TrackedStat->CombinedGraphDataSource->Initialize( Stat.Name().GetPlainNameString(), Group.ID(), Group.Name().GetPlainNameString(), Stat.Type(), ProfilerSession->GetCreationTime() );
						bMetadataInitialized = true;
					}
				}
			}
		}
	}

	return bAdded;
}

bool FProfilerManager::UntrackStat( const uint32 StatID )
{
	bool bRemoved = false;

	const FTrackedStat* TrackedStat = TrackedStats.Find( StatID );
	if( TrackedStat )
	{
		TrackedStatChangedEvent.Broadcast( *TrackedStat, false );
		TrackedStats.Remove( StatID );
		bRemoved = true;
	}

	return bRemoved;
}

void FProfilerManager::ClearStatsAndInstances()
{
	CloseAllEventGraphTabs();

	ProfilerType = EProfilerSessionTypes::InvalidOrMax;
	ViewMode = EProfilerViewMode::InvalidOrMax;
	SetDataPreview( false );
	bLivePreview = false;
	SetDataCapture( false );

	bHasCaptureFileFullyProcessed = false;

	for( auto It = TrackedStats.CreateConstIterator(); It; ++It )
	{
		const FTrackedStat& TrackedStat = It.Value();
		UntrackStat( TrackedStat.StatID );
	}

	ProfilerClient->Untrack( ActiveInstanceID );
	ActiveInstanceID.Invalidate();
}

const bool FProfilerManager::IsStatTracked( const uint32 StatID ) const
{
	return TrackedStats.Contains( StatID );
}

bool FProfilerManager::TrackStatForSessionInstance( const uint32 StatID, const FGuid& SessionInstanceID )
{
	bool bAdded = false;

	const FTrackedStat* TrackedStat = TrackedStats.Find( StatID );
	if( ProfilerSession.IsValid() && TrackedStat )
	{
		const bool bIsStatReady = ProfilerSession->GetMetaData()->IsStatInitialized( StatID );
		const bool bIsExist = TrackedStat->CombinedGraphDataSource->IsProfilerSessionRegistered( SessionInstanceID );

		if( bIsStatReady && !bIsExist )
		{
			// Create graph data provider for tracked stat.
			FGraphDataSourceRefConst	GraphDataSource = ProfilerSession->CreateGraphDataSource( StatID );
			TrackedStat->CombinedGraphDataSource->RegisterWithProfilerSession( SessionInstanceID, GraphDataSource );
			bAdded = true;
		}
	}

	return bAdded;
}

bool FProfilerManager::UntrackStatForSessionInstance( const uint32 StatID, const FGuid& SessionInstanceID )
{
	bool bRemoved = false;

	const FTrackedStat* TrackedStat = TrackedStats.Find( StatID );

	if( TrackedStat )
	{
		const bool bIsExist = TrackedStat->CombinedGraphDataSource->IsProfilerSessionRegistered( SessionInstanceID );
		if( bIsExist )
		{
			TrackedStat->CombinedGraphDataSource->UnregisterWithProfilerSession( SessionInstanceID );
			bRemoved = true;
		}
	}

	return bRemoved;
}

const bool FProfilerManager::IsStatTrackedForSessionInstance( const uint32 StatID, const FGuid& SessionInstanceID ) const
{
	bool bResult = false;
	const FTrackedStat* TrackedStat = TrackedStats.Find( StatID );
	if( TrackedStat )
	{
		bResult = (*TrackedStat).CombinedGraphDataSource->IsProfilerSessionRegistered( SessionInstanceID );
	}

	return bResult;
}


// @TODO: Move to profiler settings
const FLinearColor& FProfilerManager::GetColorForStatID( const uint32 StatID ) const
{
	static TMap<uint32,FLinearColor> StatID2ColorMapping;

	FLinearColor* Color = StatID2ColorMapping.Find( StatID );
	if( !Color )
	{
		const FColor RandomColor = FColor::MakeRandomColor();
		Color = &StatID2ColorMapping.Add( StatID, FLinearColor(RandomColor) );
	}

	return *Color;
}

void FProfilerManager::TrackDefaultStats()
{
	// Find StatId for the game thread.
	if (ProfilerSession.IsValid())
	{
		if( ProfilerSession->GetMetaData()->IsReady() )
		{
			TrackStat( ProfilerSession->GetMetaData()->GetGameThreadStatID() );
		}
	}
}

/*-----------------------------------------------------------------------------
		Event graphs management
-----------------------------------------------------------------------------*/

void FProfilerManager::CloseAllEventGraphTabs()
{
	TSharedPtr<SProfilerWindow> ProfilerWindowPtr = GetProfilerWindow();
	if( ProfilerWindowPtr.IsValid() )
	{
		// Iterate through all profiler sessions.
		if (ProfilerSession.IsValid())
		{
			ProfilerWindowPtr->ManageEventGraphTab( ProfilerSession->GetInstanceID(), false, TEXT("") );
		}

		ProfilerWindowPtr->ProfilerMiniView->Reset();
	}
}

void FProfilerManager::DataGraph_OnSelectionChangedForIndex( uint32 FrameStartIndex, uint32 FrameEndIndex )
{
	PROFILER_SCOPE_LOG_TIME( TEXT( "FProfilerManager::DataGraph_OnSelectionChangedForIndex" ), nullptr );

	if (ProfilerSession.IsValid())
	{
		FEventGraphDataRef EventGraphDataAverage = ProfilerSession->CreateEventGraphData( FrameStartIndex, FrameEndIndex, EEventGraphTypes::Average );
		FEventGraphDataRef EventGraphDataMaximum = ProfilerSession->CreateEventGraphData( FrameStartIndex, FrameEndIndex, EEventGraphTypes::Maximum );
		GetProfilerWindow()->UpdateEventGraph( ProfilerSession->GetInstanceID(), EventGraphDataAverage, EventGraphDataMaximum, false );
	}
}

void FProfilerManager::ProfilerSession_OnAddThreadTime( int32 FrameIndex, const TMap<uint32, float>& ThreadMS, const FProfilerStatMetaDataRef& StatMetaData )
{
	TSharedPtr<SProfilerWindow> ProfilerWindowPtr = GetProfilerWindow();
	if( ProfilerWindowPtr.IsValid() )
	{
		ProfilerWindowPtr->ProfilerMiniView->AddThreadTime( FrameIndex, ThreadMS, StatMetaData );
		
		// Update the notification that a file is being loaded.
		const float DataLoadingProgress = ProfilerSession->GetProgress();
		ProfilerWindowPtr->ManageLoadingProgressNotificationState( ProfilerSession->GetName(), EProfilerNotificationTypes::LoadingOfflineCapture, ELoadingProgressStates::InProgress, DataLoadingProgress );		
	}
}

void FProfilerManager::SetViewMode( EProfilerViewMode::Type NewViewMode )
{
	if( NewViewMode != ViewMode )
	{
		// Broadcast.
		OnViewModeChangedEvent.Broadcast( NewViewMode );
		ViewMode = NewViewMode;
	}
}

#undef LOCTEXT_NAMESPACE