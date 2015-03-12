// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UDemoNetDriver.cpp: Simulated network driver for recording and playing back game sessions.
=============================================================================*/

#include "EnginePrivate.h"
#include "Engine/DemoNetDriver.h"
#include "Engine/DemoNetConnection.h"
#include "Engine/DemoPendingNetGame.h"
#include "Engine/ActorChannel.h"
#include "RepLayout.h"
#include "GameFramework/SpectatorPawn.h"
#include "Engine/LevelStreamingKismet.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpectatorPawnMovement.h"
#include "Engine/GameInstance.h"
#include "NetworkReplayStreaming.h"
#include "Net/UnrealNetwork.h"
#include "Net/NetworkProfiler.h"

DEFINE_LOG_CATEGORY_STATIC( LogDemo, Log, All );

static TAutoConsoleVariable<float> CVarDemoRecordHz( TEXT( "demo.RecordHz" ), 10, TEXT( "Number of demo frames recorded per second" ) );
static TAutoConsoleVariable<float> CVarDemoTimeDilation( TEXT( "demo.TimeDilation" ), -1.0f, TEXT( "Override time dilation during demo playback (-1 = don't override)" ) );
static TAutoConsoleVariable<float> CVarDemoSkipTime( TEXT( "demo.SkipTime" ), 0, TEXT( "Skip fixed amount of network replay time (in seconds)" ) );

static const int32 MAX_DEMO_READ_WRITE_BUFFER = 1024 * 32;

#define DEMO_CHECKSUMS 0		// When setting this to 1, this will invalidate all demos, you will need to re-record and playback

/*-----------------------------------------------------------------------------
	UDemoNetDriver.
-----------------------------------------------------------------------------*/

UDemoNetDriver::UDemoNetDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UDemoNetDriver::InitBase( bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error )
{
	if ( Super::InitBase( bInitAsClient, InNotify, URL, bReuseAddressAndPort, Error ) )
	{
		DemoFilename			= URL.Map;
		Time					= 0;
		bIsRecordingDemoFrame	= false;
		bDemoPlaybackDone		= false;
		bChannelsArePaused		= false;
		TimeToSkip = 0.0f;

		ResetDemoState();

		ReplayStreamer = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();

		return true;
	}

	return false;
}

void UDemoNetDriver::FinishDestroy()
{
	if ( !HasAnyFlags( RF_ClassDefaultObject ) )
	{
		// Make sure we stop any recording that might be going on
		if ( ClientConnections.Num() > 0 )
		{
			StopDemo();
		}
	}

	Super::FinishDestroy();
}

FString UDemoNetDriver::LowLevelGetNetworkNumber()
{
	return FString( TEXT( "" ) );
}

enum ENetworkVersionHistory
{
	HISTORY_INITIAL				= 1,
	HISTORY_SAVE_ABS_TIME_MS	= 2,			// We now save the abs demo time in ms for each frame (solves accumulation errors)
	HISTORY_INCREASE_BUFFER		= 3				// Increased buffer size of packets, which invalidates old replays
};

static const uint32 NETWORK_DEMO_MAGIC				= 0x2CF5A13D;
static const uint32 NETWORK_DEMO_VERSION			= HISTORY_INCREASE_BUFFER;

static const uint32 NETWORK_DEMO_METADATA_MAGIC		= 0x3D06B24E;
static const uint32 NETWORK_DEMO_METADATA_VERSION	= 0;

struct FNetworkDemoHeader
{
	uint32	Magic;					// Magic to ensure we're opening the right file.
	uint32	Version;				// Version number to detect version mismatches.
	uint32	EngineNetVersion;		// Version of engine networking format
	FString LevelName;				// Name of level loaded for demo
	
	FNetworkDemoHeader() : 
		Magic( NETWORK_DEMO_MAGIC ), 
		Version( NETWORK_DEMO_VERSION ),
		EngineNetVersion( GEngineNetVersion )
	{}

	friend FArchive& operator << ( FArchive& Ar, FNetworkDemoHeader& Header )
	{
		Ar << Header.Magic;
		Ar << Header.Version;
		Ar << Header.LevelName;

		return Ar;
	}
};

struct FNetworkDemoMetadataHeader
{
	uint32	Magic;					// Magic to ensure we're opening the right file.
	uint32	Version;				// Version number to detect version mismatches.
	uint32	EngineNetVersion;		// Version of engine networking format
	int32	NumFrames;				// Number of total frames in the demo
	float	TotalTime;				// Number of total time in seconds in demo
	int32	NumStreamingLevels;		// Number of streaming levels

	FNetworkDemoMetadataHeader() : 
		Magic( NETWORK_DEMO_METADATA_MAGIC ), 
		Version( NETWORK_DEMO_METADATA_VERSION ),
		EngineNetVersion( GEngineNetVersion ),
		NumFrames( 0 ),
		TotalTime( 0 ),
		NumStreamingLevels( 0 )
	{}
	
	friend FArchive& operator << ( FArchive& Ar, FNetworkDemoMetadataHeader& Header )
	{
		Ar << Header.Magic;
		Ar << Header.Version;
		Ar << Header.NumFrames;
		Ar << Header.TotalTime;
		Ar << Header.NumStreamingLevels;

		return Ar;
	}
};

void UDemoNetDriver::ResetDemoState()
{
	DemoFrameNum	= 0;
	LastRecordTime	= 0;
	DemoTotalTime	= 0;
	DemoCurrentTime	= 0;
	DemoTotalFrames	= 0;
}

bool UDemoNetDriver::InitConnect( FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error )
{
	if ( GetWorld() == nullptr )
	{
		UE_LOG( LogDemo, Error, TEXT( "GetWorld() == nullptr" ) );
		return false;
	}

	if ( GetWorld()->GetGameInstance() == nullptr )
	{
		UE_LOG( LogDemo, Error, TEXT( "GetWorld()->GetGameInstance() == nullptr" ) );
		return false;
	}

	// handle default initialization
	if ( !InitBase( true, InNotify, ConnectURL, false, Error ) )
	{
		GetWorld()->GetGameInstance()->HandleDemoPlaybackFailure( EDemoPlayFailure::Generic, FString( TEXT( "InitBase FAILED" ) ) );
		return false;
	}

	// Playback, local machine is a client, and the demo stream acts "as if" it's the server.
	ServerConnection = NewObject<UNetConnection>(GetTransientPackage(), UDemoNetConnection::StaticClass());
	ServerConnection->InitConnection( this, USOCK_Pending, ConnectURL, 1000000 );

	FString VersionString = FString::Printf( TEXT( "%s_%u" ), FApp::GetGameName(), FNetworkVersion::GetLocalNetworkVersion() );

	ReplayStreamer->StartStreaming( DemoFilename, false, VersionString, FOnStreamReadyDelegate::CreateUObject( this, &UDemoNetDriver::ReplayStreamingReady ) );

	return true;
}

bool UDemoNetDriver::InitConnectInternal( FString& Error )
{
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();

	ResetDemoState();

	FArchive* FileAr = ReplayStreamer->GetHeaderArchive();

	if ( !FileAr )
	{
		Error = FString::Printf( TEXT( "Couldn't open demo file %s for reading" ), *DemoFilename );
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: %s" ), *Error );
		GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::DemoNotFound, FString( EDemoPlayFailure::ToString( EDemoPlayFailure::DemoNotFound ) ) );
		return false;
	}

	FNetworkDemoHeader DemoHeader;

	(*FileAr) << DemoHeader;

	// Check magic value
	if ( DemoHeader.Magic != NETWORK_DEMO_MAGIC )
	{
		Error = FString( TEXT( "Demo file is corrupt" ) );
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: %s" ), *Error );
		GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::Corrupt, Error );
		return false;
	}

	// Check version
	if ( DemoHeader.Version != NETWORK_DEMO_VERSION )
	{
		Error = FString( TEXT( "Demo file version is incorrect" ) );
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: %s" ), *Error );
		GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::InvalidVersion, Error );
		return false;
	}

	// Create fake control channel
	ServerConnection->CreateChannel( CHTYPE_Control, 1 );

	// Attempt to read metadata if it exists
	FArchive* MetadataAr = ReplayStreamer->GetMetadataArchive();

	FNetworkDemoMetadataHeader MetadataHeader;

	if ( MetadataAr != nullptr )
	{
		(*MetadataAr) << MetadataHeader;

		// Check metadata magic value
		if ( MetadataHeader.Magic != NETWORK_DEMO_METADATA_MAGIC )
		{
			Error = FString( TEXT( "Demo metadata file is corrupt" ) );
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: %s" ), *Error );
			GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::Corrupt, Error );
			return false;
		}

		// Check version
		if ( MetadataHeader.Version != NETWORK_DEMO_METADATA_VERSION )
		{
			Error = FString( TEXT( "Demo metadata file version is incorrect" ) );
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: %s" ), *Error );
			GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::InvalidVersion, Error );
			return false;
		}

		DemoTotalFrames = MetadataHeader.NumFrames;
		DemoTotalTime	= MetadataHeader.TotalTime;

		UE_LOG( LogDemo, Log, TEXT( "Starting demo playback with full demo and metadata. Filename: %s, Frames: %i, Version %i" ), *DemoFilename, DemoTotalFrames, DemoHeader.Version );
	}
	else
	{
		UE_LOG( LogDemo, Log, TEXT( "Starting demo playback with streaming demo, metadata file not found. Filename: %s, Version %i" ), *DemoFilename, DemoHeader.Version );
	}
	
	// Bypass UDemoPendingNetLevel
	FString LoadMapError;

	FURL DemoURL;
	DemoURL.Map = DemoHeader.LevelName;

	FWorldContext * WorldContext = GEngine->GetWorldContextFromWorld( GetWorld() );

	if ( WorldContext == NULL )
	{
		Error = FString::Printf( TEXT( "No world context" ), *DemoFilename );
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: %s" ), *Error );
		GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::Generic, FString( TEXT( "No world context" ) ) );
		return false;
	}

	GetWorld()->DemoNetDriver = NULL;
	SetWorld( NULL );

	auto NewPendingNetGame = NewObject<UDemoPendingNetGame>();

	NewPendingNetGame->DemoNetDriver = this;

	WorldContext->PendingNetGame = NewPendingNetGame;

	if ( !GEngine->LoadMap( *WorldContext, DemoURL, NewPendingNetGame, LoadMapError ) )
	{
		Error = LoadMapError;
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::InitConnect: LoadMap failed: failed: %s" ), *Error );
		GameInstance->HandleDemoPlaybackFailure( EDemoPlayFailure::Generic, FString( TEXT( "LoadMap failed" ) ) );
		return false;
	}

	SetWorld( WorldContext->World() );
	WorldContext->World()->DemoNetDriver = this;
	WorldContext->PendingNetGame = NULL;

	// Read meta data, if it exists
	for ( int32 i = 0; i < MetadataHeader.NumStreamingLevels; ++i )
	{
		ULevelStreamingKismet* StreamingLevel = NewObject<ULevelStreamingKismet>(GetWorld(), NAME_None, RF_NoFlags, NULL);

		StreamingLevel->bShouldBeLoaded		= true;
		StreamingLevel->bShouldBeVisible	= true;
		StreamingLevel->bShouldBlockOnLoad	= false;
		StreamingLevel->bInitiallyLoaded	= true;
		StreamingLevel->bInitiallyVisible	= true;

		FString PackageName;
		FString PackageNameToLoad;

		(*MetadataAr) << PackageName;
		(*MetadataAr) << PackageNameToLoad;
		(*MetadataAr) << StreamingLevel->LevelTransform;

		StreamingLevel->PackageNameToLoad = FName( *PackageNameToLoad );
		StreamingLevel->SetWorldAssetByPackageName( FName( *PackageName ) );

		GetWorld()->StreamingLevels.Add( StreamingLevel );

		UE_LOG( LogDemo, Log, TEXT( "  Loading streamingLevel: %s, %s" ), *PackageName, *PackageNameToLoad );
	}

	return true;
}

bool UDemoNetDriver::InitListen( FNetworkNotify* InNotify, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error )
{
	if ( !InitBase( false, InNotify, ListenURL, bReuseAddressAndPort, Error ) )
	{
		return false;
	}

	check( World != NULL );

	class AWorldSettings * WorldSettings = World->GetWorldSettings(); 

	if ( !WorldSettings )
	{
		Error = TEXT( "No WorldSettings!!" );
		return false;
	}

	// Recording, local machine is server, demo stream acts "as if" it's a client.
	UDemoNetConnection* Connection = NewObject<UDemoNetConnection>();
	Connection->InitConnection( this, USOCK_Open, ListenURL, 1000000 );
	Connection->InitSendBuffer();
	ClientConnections.Add( Connection );

	FString VersionString = FString::Printf( TEXT( "%s_%u" ), FApp::GetGameName(), FNetworkVersion::GetLocalNetworkVersion() );

	ReplayStreamer->StartStreaming( DemoFilename, true, VersionString, FOnStreamReadyDelegate::CreateUObject( this, &UDemoNetDriver::ReplayStreamingReady ) );

	FArchive* FileAr = ReplayStreamer->GetHeaderArchive();

	if( !FileAr )
	{
		Error = FString::Printf( TEXT("Couldn't open demo file %s for writing"), *DemoFilename );//@todo demorec: localize
		return false;
	}

	// use the same byte format regardless of platform so that the demos are cross platform
	//@note: swap on non console platforms as the console archives have byte swapping compiled out by default
	//FileAr->SetByteSwapping(true);

	FNetworkDemoHeader DemoHeader;

	DemoHeader.LevelName = World->GetCurrentLevel()->GetOutermost()->GetName();

	// Write the header
	(*FileAr) << DemoHeader;

	// Spawn the demo recording spectator.
	SpawnDemoRecSpectator( Connection );

	return true;
}

void UDemoNetDriver::TickDispatch( float DeltaSeconds )
{
	Super::TickDispatch( DeltaSeconds );
}

void UDemoNetDriver::TickFlush( float DeltaSeconds )
{
	Super::TickFlush( DeltaSeconds );

	if ( ClientConnections.Num() == 0 && ServerConnection == nullptr )
	{
		// Nothing to do
		return;
	}

	if ( ReplayStreamer->GetLastError() != ENetworkReplayError::None )
	{
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::TickFlush: ReplayStreamer ERROR: %s" ), ENetworkReplayError::ToString( ReplayStreamer->GetLastError() ) );
		StopDemo();
		return;
	}

	FArchive* FileAr = ReplayStreamer->GetStreamingArchive();

	if ( FileAr == nullptr )
	{
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::TickFlush: FileAr == nullptr" ) );
		StopDemo();
		return;
	}

	if ( ClientConnections.Num() > 0 )
	{
		const double StartTime = FPlatformTime::Seconds();

		TickDemoRecord( DeltaSeconds );

		const double EndTime = FPlatformTime::Seconds();

		const double RecordTotalTime = ( EndTime - StartTime );

		MaxRecordTime = FMath::Max( MaxRecordTime, RecordTotalTime );

		AccumulatedRecordTime += RecordTotalTime;

		RecordCountSinceFlush++;

		const double ElapsedTime = EndTime - LastRecordAvgFlush;

		const double AVG_FLUSH_TIME_IN_SECONDS = 2;

		if ( ElapsedTime > AVG_FLUSH_TIME_IN_SECONDS && RecordCountSinceFlush > 0 )
		{
			const float AvgTimeMS = ( AccumulatedRecordTime / RecordCountSinceFlush ) * 1000;
			const float MaxRecordTimeMS = MaxRecordTime * 1000;

			if ( AvgTimeMS > 3.0f || MaxRecordTimeMS > 6.0f )
			{
				UE_LOG( LogDemo, Warning, TEXT( "UDemoNetDriver::TickFlush: SLOW FRAME. Avg: %2.2f, Max: %2.2f, Actors: %i" ), AvgTimeMS, MaxRecordTimeMS, World->NetworkActors.Num() );
			}

			LastRecordAvgFlush		= EndTime;
			AccumulatedRecordTime	= 0;
			MaxRecordTime			= 0;
			RecordCountSinceFlush	= 0;
		}
	}
	else if ( ServerConnection != NULL )
	{
		// Wait until all levels are streamed in
		for ( int32 i = 0; i < World->StreamingLevels.Num(); ++i )
		{
			ULevelStreaming * StreamingLevel = World->StreamingLevels[i];

			if ( StreamingLevel != NULL && StreamingLevel->ShouldBeLoaded() && (!StreamingLevel->IsLevelLoaded() || !StreamingLevel->GetLoadedLevel()->GetOutermost()->IsFullyLoaded() || !StreamingLevel->IsLevelVisible() ) )
			{
				// Abort, we have more streaming levels to load
				return;
			}
		}

		if ( CVarDemoTimeDilation.GetValueOnGameThread() >= 0.0f )
		{
			World->GetWorldSettings()->DemoPlayTimeDilation = CVarDemoTimeDilation.GetValueOnGameThread();
		}

		// Clamp time between 1000 hz, and 2 hz 
		// (this is useful when debugging and you set a breakpoint, you don't want all that time to pass in one frame)
		DeltaSeconds = FMath::Clamp( DeltaSeconds, 1.0f / 1000.0f, 1.0f / 2.0f );

		// We need to compensate for the fact that DeltaSeconds is real-time for net drivers
		DeltaSeconds *= World->GetWorldSettings()->GetEffectiveTimeDilation();

		// Update time dilation on spectator pawn to compensate for any demo dilation 
		//	(we want to continue to fly around in real-time)
		if ( SpectatorController != NULL )
		{
			if ( SpectatorController->GetSpectatorPawn() != NULL )
			{
				// Disable collision on the spectator
				SpectatorController->GetSpectatorPawn()->SetActorEnableCollision( false );
					
				SpectatorController->GetSpectatorPawn()->PrimaryActorTick.bTickEvenWhenPaused = true;

				USpectatorPawnMovement* SpectatorMovement = Cast<USpectatorPawnMovement>(SpectatorController->GetSpectatorPawn()->GetMovementComponent());

				if ( SpectatorMovement )
				{
					SpectatorMovement->bIgnoreTimeDilation = true;
					SpectatorMovement->PrimaryComponentTick.bTickEvenWhenPaused = true;
				}
			}
		}

		if ( bDemoPlaybackDone )
		{
			return;
		}

		if ( World->GetWorldSettings()->Pauser == NULL )
		{
			TickDemoPlayback( DeltaSeconds );
		}
	}
}

void UDemoNetDriver::ProcessRemoteFunction( class AActor* Actor, class UFunction* Function, void* Parameters, struct FOutParmRec* OutParms, struct FFrame* Stack, class UObject* SubObject )
{
	if (ClientConnections.Num() > 0 && ClientConnections[0] != NULL)
	{
		if ((Function->FunctionFlags & FUNC_NetMulticast))
		{
			InternalProcessRemoteFunction(Actor, SubObject, ClientConnections[0], Function, Parameters, OutParms, Stack, IsServer());
		}
	}
}

bool UDemoNetDriver::Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar )
{
	return Super::Exec( InWorld, Cmd, Ar);
}

void UDemoNetDriver::StopDemo()
{
	if ( !ServerConnection && ClientConnections.Num() == 0 )
	{
		UE_LOG( LogDemo, Warning, TEXT( "StopDemo: No demo is playing" ) );
		return;
	}

	UE_LOG( LogDemo, Log, TEXT( "StopDemo: Demo %s stopped at frame %d" ), *DemoFilename, DemoFrameNum );

	if ( !ServerConnection )
	{
		FArchive* MetadataAr = ReplayStreamer->GetMetadataArchive();

		// Finish writing the metadata
		if ( MetadataAr != NULL && World != NULL )
		{
			DemoTotalFrames = DemoFrameNum;
			DemoTotalTime	= DemoCurrentTime;

			// Get the number of streaming levels so we can update the metadata with the correct info
			int32 NumStreamingLevels = 0;

			for ( int32 i = 0; i < World->StreamingLevels.Num(); ++i )
			{
				if ( World->StreamingLevels[i] != NULL )
				{
					NumStreamingLevels++;
				}
			}

			// Make a header for the metadata
			FNetworkDemoMetadataHeader DemoHeader;

			DemoHeader.NumFrames			= DemoTotalFrames;
			DemoHeader.TotalTime			= DemoTotalTime;
			DemoHeader.NumStreamingLevels	= NumStreamingLevels;

			// Seek to beginning
			MetadataAr->Seek( 0 );

			// Write header
			(*MetadataAr) << DemoHeader;

			//
			// Write meta data
			//

			// Save out any levels that are in the streamed level list
			// This needs some work, but for now, to try and get games that use heavy streaming working
			for ( int32 i = 0; i < World->StreamingLevels.Num(); ++i )
			{
				if ( World->StreamingLevels[i] != NULL )
				{
					FString PackageName = World->StreamingLevels[i]->GetWorldAssetPackageName();
					FString PackageNameToLoad = World->StreamingLevels[i]->PackageNameToLoad.ToString();

					UE_LOG( LogDemo, Log, TEXT( "  StreamingLevel: %s, %s" ), *PackageName, *PackageNameToLoad );

					(*MetadataAr) << PackageName;
					(*MetadataAr) << PackageNameToLoad;
					(*MetadataAr) << World->StreamingLevels[i]->LevelTransform;
				}
			}
		}

		// let GC cleanup the object
		if ( ClientConnections.Num() > 0 && ClientConnections[0] != NULL )
		{
			ClientConnections[0]->Close();
			ClientConnections[0]->CleanUp(); // make sure DemoRecSpectator gets destroyed immediately
		}
	}
	else
	{
		// flush out any pending network traffic
		ServerConnection->FlushNet();

		ServerConnection->State = USOCK_Closed;
		ServerConnection->Close();
		ServerConnection->CleanUp(); // make sure DemoRecSpectator gets destroyed immediately
		ServerConnection = NULL;
	}

	ReplayStreamer->StopStreaming();

	check( ClientConnections.Num() == 0 );
	check( ServerConnection == NULL );
}

/*-----------------------------------------------------------------------------
Demo Recording tick.
-----------------------------------------------------------------------------*/

static void DemoReplicateActor(AActor* Actor, UNetConnection* Connection, bool IsNetClient)
{
	// All actors marked for replication are assumed to be relevant for demo recording.
	/*
	if
		(	Actor
		&&	((IsNetClient && Actor->bTearOff) || Actor->RemoteRole != ROLE_None || (IsNetClient && Actor->Role != ROLE_None && Actor->Role != ROLE_Authority) || Actor->bForceDemoRelevant)
		&&  (!Actor->bNetTemporary || !Connection->SentTemporaries.Contains(Actor))
		&& (Actor == Connection->PlayerController || Cast<APlayerController>(Actor) == NULL)
		)
	*/
	if ( Actor != NULL && Actor->GetRemoteRole() != ROLE_None && ( Actor == Connection->PlayerController || Cast< APlayerController >( Actor ) == NULL ) )
	{
		if (Actor->bRelevantForNetworkReplays)
		{
			// Create a new channel for this actor.
			const bool StartupActor = Actor->IsNetStartupActor();
			UActorChannel* Channel = Connection->ActorChannels.FindRef(Actor);

			if (!Channel && (!StartupActor || Connection->ClientHasInitializedLevelFor(Actor)))
			{
				// create a channel if possible
				Channel = (UActorChannel*)Connection->CreateChannel(CHTYPE_Actor, 1);
				if (Channel != NULL)
				{
					Channel->SetChannelActor(Actor);
				}
			}

			if (Channel)
			{
				// Send it out!
				check(!Channel->Closing);

				if (Channel->IsNetReady(0))
				{
					Channel->ReplicateActor();
				}
			}
		}
	}
}

void UDemoNetDriver::TickDemoRecord( float DeltaSeconds )
{
	if ( ClientConnections.Num() == 0 )
	{
		return;
	}

	FArchive* FileAr = ReplayStreamer->GetStreamingArchive();

	if ( FileAr == NULL )
	{
		return;
	}

	DemoCurrentTime += DeltaSeconds;

	ReplayStreamer->UpdateTotalDemoTime( GetDemoCurrentTimeInMS() );

	const double CurrentSeconds = FPlatformTime::Seconds();

	const double RECORD_HZ		= CVarDemoRecordHz.GetValueOnGameThread();
	const double RECORD_DELAY	= 1.0 / RECORD_HZ;

	if ( CurrentSeconds - LastRecordTime < RECORD_DELAY )
	{
		return;		// Not enough real-time has passed to record another frame
	}

	LastRecordTime = CurrentSeconds;

	// Save out a frame
	DemoFrameNum++;
	ReplicationFrame++;

	// Save total absolute demo time in MS
	uint32 SavedAbsTimeMS = GetDemoCurrentTimeInMS();
	*FileAr << SavedAbsTimeMS;

#if DEMO_CHECKSUMS == 1
	uint32 DeltaTimeChecksum = FCrc::MemCrc32( &SavedAbsTimeMS, sizeof( SavedAbsTimeMS ), 0 );
	*FileAr << DeltaTimeChecksum;
#endif

	// Make sure we don't have anything in the buffer for this new frame
	check( ClientConnections[0]->SendBuffer.GetNumBits() == 0 );

	bIsRecordingDemoFrame = true;

	// Dump any queued packets
	UDemoNetConnection * ClientDemoConnection = CastChecked< UDemoNetConnection >( ClientConnections[0] );

	for ( int32 i = 0; i < ClientDemoConnection->QueuedDemoPackets.Num(); i++ )
	{
		ClientDemoConnection->LowLevelSend( (char*)&ClientDemoConnection->QueuedDemoPackets[i].Data[0], ClientDemoConnection->QueuedDemoPackets[i].Data.Num() );
	}

	ClientDemoConnection->QueuedDemoPackets.Empty();

	const bool IsNetClient = ( GetWorld()->GetNetDriver() != NULL && GetWorld()->GetNetDriver()->GetNetMode() == NM_Client );

	DemoReplicateActor( World->GetWorldSettings(), ClientConnections[0], IsNetClient );

	for ( int32 i = 0; i < World->NetworkActors.Num(); i++ )
	{
		AActor* Actor = World->NetworkActors[i];

		Actor->PreReplication( *FindOrCreateRepChangedPropertyTracker( Actor ).Get() );
		DemoReplicateActor( Actor, ClientConnections[0], IsNetClient );
	}

	// Make sure nothing is left over
	ClientConnections[0]->FlushNet();

	check( ClientConnections[0]->SendBuffer.GetNumBits() == 0 );

	bIsRecordingDemoFrame = false;

	// Write a count of 0 to signal the end of the frame
	int32 EndCount = 0;

	*FileAr << EndCount;
}

void UDemoNetDriver::PauseChannels( const bool bPause )
{
	if ( bPause == bChannelsArePaused )
	{
		return;
	}

	// Pause all non player controller actors
	for ( int32 i = ServerConnection->OpenChannels.Num() - 1; i >= 0; i-- )
	{
		UChannel* OpenChannel = ServerConnection->OpenChannels[i];
		if ( OpenChannel != NULL )
		{
			UActorChannel* ActorChannel = Cast< UActorChannel >( OpenChannel );
			if ( ActorChannel != NULL && ActorChannel->GetActor() != NULL )
			{
				if ( Cast< APlayerController >( ActorChannel->GetActor() ) == NULL )
				{
					// Better way to pause each actor?
					ActorChannel->GetActor()->CustomTimeDilation = bPause ? 0.0f : 1.0f;
				}
			}
		}
	}

	bChannelsArePaused = bPause;
}

bool UDemoNetDriver::ReadDemoFrame()
{
	FArchive* FileAr = ReplayStreamer->GetStreamingArchive();

	if ( FileAr->IsError() )
	{
		StopDemo();
		return false;
	}

	if ( FileAr->AtEnd() )
	{
		bDemoPlaybackDone = true;
		PauseChannels( true );
		return false;
	}

	if ( ReplayStreamer->GetLastError() != ENetworkReplayError::None )
	{
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: ReplayStreamer ERROR: %s" ), ENetworkReplayError::ToString( ReplayStreamer->GetLastError() ) );
		StopDemo();
		return false;
	}

	if ( !ReplayStreamer->IsDataAvailable() )
	{
		PauseChannels( true );
		return false;
	} 

	PauseChannels( false );

	const int32 OldFilePos = FileAr->Tell();

	uint32 SavedAbsTimeMS = 0;

	// Peek at the next demo delta time, and see if we should process this frame
	*FileAr << SavedAbsTimeMS;

#if DEMO_CHECKSUMS == 1
	{
		uint32 ServerDeltaTimeCheksum = 0;
		*FileAr << ServerDeltaTimeCheksum;

		const uint32 DeltaTimeChecksum = FCrc::MemCrc32( &SavedAbsTimeMS, sizeof( SavedAbsTimeMS ), 0 );

		if ( DeltaTimeChecksum != ServerDeltaTimeCheksum )
		{
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: DeltaTimeChecksum != ServerDeltaTimeCheksum" ) );
			StopDemo();
			return false;
		}
	}
#endif

	if ( FileAr->IsError() )
	{
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: Failed to read demo ServerDeltaTime" ) );
		StopDemo();
		return false;
	}

	if ( GetDemoCurrentTimeInMS() < SavedAbsTimeMS )
	{
		// Not enough time has passed to read another frame
		FileAr->Seek( OldFilePos );
		return false;
	}

	while ( true )
	{
		uint8 ReadBuffer[ MAX_DEMO_READ_WRITE_BUFFER ];

		int32 PacketBytes;

		*FileAr << PacketBytes;

		if ( FileAr->IsError() )
		{
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: Failed to read demo PacketBytes" ) );
			StopDemo();
			return false;
		}

		if ( PacketBytes == 0 )
		{
			break;
		}

		if ( PacketBytes > sizeof( ReadBuffer ) )
		{
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: PacketBytes > sizeof( ReadBuffer )" ) );

			StopDemo();

			if ( World != NULL && World->GetGameInstance() != NULL )
			{
				World->GetGameInstance()->HandleDemoPlaybackFailure( EDemoPlayFailure::Generic, FString( TEXT( "UDemoNetDriver::ReadDemoFrame: PacketBytes > sizeof( ReadBuffer )" ) ) );
			}

			return false;
		}

		// Read data from file.
		FileAr->Serialize( ReadBuffer, PacketBytes );

		if ( FileAr->IsError() )
		{
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: Failed to read demo file packet" ) );
			StopDemo();
			return false;
		}

#if DEMO_CHECKSUMS == 1
		{
			uint32 ServerChecksum = 0;
			*FileAr << ServerChecksum;

			const uint32 Checksum = FCrc::MemCrc32( ReadBuffer, PacketBytes, 0 );

			if ( Checksum != ServerChecksum )
			{
				UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: Checksum != ServerChecksum" ) );
				StopDemo();
				return false;
			}
		}
#endif

		// Process incoming packet.
		ServerConnection->ReceivedRawPacket( ReadBuffer, PacketBytes );

		if ( ServerConnection == NULL || ServerConnection->State == USOCK_Closed )
		{
			// Something we received resulted in the demo being stopped
			UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::ReadDemoFrame: ReceivedRawPacket closed connection" ) );
			StopDemo();
			return false;
		}
	}

	return true;
}

void UDemoNetDriver::SkipTime(float InTimeToSkip)
{
	TimeToSkip = InTimeToSkip;
}

void UDemoNetDriver::TickDemoPlayback( float DeltaSeconds )
{
	if ( ServerConnection == NULL || ServerConnection->State == USOCK_Closed )
	{
		StopDemo();
		return;
	}

	const uint32 TotalDemoTimeInMS = ReplayStreamer->GetTotalDemoTime();

	if ( TotalDemoTimeInMS > 0 )
	{
		DemoTotalTime = (float)TotalDemoTimeInMS / 1000.0f;
	}

	if ( CVarDemoSkipTime.GetValueOnGameThread() > 0 )
	{
		SkipTime( CVarDemoSkipTime.GetValueOnGameThread() );		// Just overwrite existing value, cvar wins in this case
		CVarDemoSkipTime.AsVariable()->Set( TEXT( "0" ), ECVF_SetByConsole );
	}
	
	if ( TimeToSkip > 0.0f )
	{
		ReplayStreamer->SetHighPriorityTimeRange( 0, DemoTotalTime );		// HACK for now, download the entire demo until we get support to download to any time

		if ( !ReplayStreamer->IsDataAvailableForTimeRange( 0, DemoTotalTime ) )
		{
			PauseChannels( true );
			return;
		}

		PauseChannels( false );

		DemoCurrentTime += TimeToSkip;

		TimeToSkip = 0.0f;
	}

	DemoCurrentTime += DeltaSeconds;

	if ( DemoCurrentTime > DemoTotalTime )
	{
		DemoCurrentTime = DemoTotalTime;
	}

	// Read demo frames until we are caught up
	while ( ReadDemoFrame() )
	{
		DemoFrameNum++;
	}
}

void UDemoNetDriver::SpawnDemoRecSpectator( UNetConnection* Connection )
{
	check( Connection != NULL );

	UClass* C = StaticLoadClass( AActor::StaticClass(), NULL, *DemoSpectatorClass, NULL, LOAD_None, NULL );

	if ( C == NULL )
	{
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::SpawnDemoRecSpectator: Failed to load demo spectator class." ) );
		return;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.ObjectFlags |= RF_Transient;	// We never want these to save into a map
	APlayerController* Controller = World->SpawnActor<APlayerController>( C, SpawnInfo );

	if ( Controller == NULL )
	{
		UE_LOG( LogDemo, Error, TEXT( "UDemoNetDriver::SpawnDemoRecSpectator: Failed to spawn demo spectator." ) );
		return;
	}

	for ( FActorIterator It( World ); It; ++It)
	{
		if ( It->IsA( APlayerStart::StaticClass() ) )
		{
			Controller->SetInitialLocationAndRotation( It->GetActorLocation(), It->GetActorRotation() );
			break;
		}
	}

	Controller->SetReplicates( true );
	Controller->SetAutonomousProxy( true );

	Controller->SetPlayer( Connection );
}

void UDemoNetDriver::ReplayStreamingReady( bool bSuccess, bool bRecord )
{
	if ( !bSuccess )
	{
		GetWorld()->GetGameInstance()->HandleDemoPlaybackFailure( EDemoPlayFailure::DemoNotFound, FString( EDemoPlayFailure::ToString( EDemoPlayFailure::DemoNotFound ) ) );
		return;
	}

	if ( !bRecord )
	{
		FString Error;
		InitConnectInternal( Error );
	}
}

/*-----------------------------------------------------------------------------
	UDemoNetConnection.
-----------------------------------------------------------------------------*/

UDemoNetConnection::UDemoNetConnection( const FObjectInitializer& ObjectInitializer ) : Super( ObjectInitializer )
{
	MaxPacket = MAX_DEMO_READ_WRITE_BUFFER;
	InternalAck = true;
}

void UDemoNetConnection::InitConnection( UNetDriver* InDriver, EConnectionState InState, const FURL& InURL, int32 InConnectionSpeed )
{
	// default implementation
	Super::InitConnection( InDriver, InState, InURL, InConnectionSpeed );

	MaxPacket = MAX_DEMO_READ_WRITE_BUFFER;
	InternalAck = true;

	InitSendBuffer();

	// the driver must be a DemoRecording driver (GetDriver makes assumptions to avoid Cast'ing each time)
	check( InDriver->IsA( UDemoNetDriver::StaticClass() ) );
}

FString UDemoNetConnection::LowLevelGetRemoteAddress( bool bAppendPort )
{
	return TEXT( "UDemoNetConnection" );
}

void UDemoNetConnection::LowLevelSend( void* Data, int32 Count )
{
	if ( Count == 0 )
	{
		UE_LOG( LogDemo, Warning, TEXT( "UDemoNetConnection::LowLevelSend: Ignoring empty packet." ) );
		return;
	}

	if ( Count > MAX_DEMO_READ_WRITE_BUFFER )
	{
		UE_LOG( LogDemo, Fatal, TEXT( "UDemoNetConnection::LowLevelSend: Count > MAX_DEMO_READ_WRITE_BUFFER." ) );
	}

	FArchive* FileAr = GetDriver()->ReplayStreamer->GetStreamingArchive();

	if ( !GetDriver()->ServerConnection && FileAr )
	{
		// If we're outside of an official demo frame, we need to queue this up or it will throw off the stream
		if ( !GetDriver()->bIsRecordingDemoFrame )
		{
			FQueuedDemoPacket & B = *( new( QueuedDemoPackets )FQueuedDemoPacket );
			B.Data.AddUninitialized( Count );
			FMemory::Memcpy( B.Data.GetData(), Data, Count );
			return;
		}

		*FileAr << Count;
		FileAr->Serialize( Data, Count );
		
		NETWORK_PROFILER(GNetworkProfiler.FlushOutgoingBunches(this));

#if DEMO_CHECKSUMS == 1
		uint32 Checksum = FCrc::MemCrc32( Data, Count, 0 );
		*FileAr << Checksum;
#endif
	}
}

FString UDemoNetConnection::LowLevelDescribe()
{
	return TEXT( "Demo recording/playback driver connection" );
}

int32 UDemoNetConnection::IsNetReady( bool Saturate )
{
	return 1;
}

void UDemoNetConnection::FlushNet( bool bIgnoreSimulation )
{
	// in playback, there is no data to send except
	// channel closing if an error occurs.
	if ( GetDriver()->ServerConnection != NULL )
	{
		InitSendBuffer();
	}
	else
	{
		Super::FlushNet( bIgnoreSimulation );
	}
}

void UDemoNetConnection::HandleClientPlayer( APlayerController* PC, UNetConnection* NetConnection )
{
	Super::HandleClientPlayer( PC, NetConnection );

	// Assume this is our special spectator controller
	GetDriver()->SpectatorController = PC;

	for ( FActorIterator It( Driver->World ); It; ++It)
	{
		if ( It->IsA( APlayerStart::StaticClass() ) )
		{
			PC->SetInitialLocationAndRotation( It->GetActorLocation(), It->GetActorRotation() );

			if ( PC->GetPawn() )
			{
				PC->GetPawn()->TeleportTo( It->GetActorLocation(), It->GetActorRotation(), false, true );
			}
			break;
		}
	}
}

bool UDemoNetConnection::ClientHasInitializedLevelFor(const UObject* TestObject) const
{
	// We save all currently streamed levels into the demo stream so we can force the demo playback client
	// to stay in sync with the recording server
	// This may need to be tweaked or re-evaluated when we start recording demos on the client
	return ( GetDriver()->DemoFrameNum > 2 || Super::ClientHasInitializedLevelFor( TestObject ) );
}

/*-----------------------------------------------------------------------------
	UDemoPendingNetGame.
-----------------------------------------------------------------------------*/

UDemoPendingNetGame::UDemoPendingNetGame( const FObjectInitializer& ObjectInitializer ) : Super( ObjectInitializer )
{
}
