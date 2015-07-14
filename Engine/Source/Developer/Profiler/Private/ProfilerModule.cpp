// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ProfilerPrivatePCH.h"
#include "SDockTab.h"

/**
 * Implements the FProfilerModule module.
 */
class FProfilerModule
	: public IProfilerModule
{
public:
	virtual TSharedRef<SWidget> CreateProfilerWindow( const ISessionManagerRef& InSessionManager, const TSharedRef<SDockTab>& ConstructUnderMajorTab ) override;

	virtual void StartupModule() override
	{}

	virtual void ShutdownModule() override;

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	void StatsMemoryDumpCommand( const TCHAR* Filename );

	FRawStatsMemoryProfiler* OpenRawStatsForMemoryProfiling( const TCHAR* Filename );

protected:
	/** Shutdowns the profiler manager. */
	void Shutdown( TSharedRef<SDockTab> TabBeingClosed );
};

IMPLEMENT_MODULE( FProfilerModule, Profiler );

/*-----------------------------------------------------------------------------
	FProfilerModule
-----------------------------------------------------------------------------*/


TSharedRef<SWidget> FProfilerModule::CreateProfilerWindow( const ISessionManagerRef& InSessionManager, const TSharedRef<SDockTab>& ConstructUnderMajorTab )
{
	FProfilerManager::Initialize( InSessionManager );
	TSharedRef<SProfilerWindow> ProfilerWindow = SNew( SProfilerWindow );
	FProfilerManager::Get()->AssignProfilerWindow( ProfilerWindow );
	// Register OnTabClosed to handle profiler manager shutdown.
	ConstructUnderMajorTab->SetOnTabClosed( SDockTab::FOnTabClosedCallback::CreateRaw( this, &FProfilerModule::Shutdown ) );

	return ProfilerWindow;
}

void FProfilerModule::ShutdownModule()
{
	if (FProfilerManager::Get().IsValid())
	{
		FProfilerManager::Get()->Shutdown();
	}
}

void FProfilerModule::Shutdown( TSharedRef<SDockTab> TabBeingClosed )
{
	FProfilerManager::Get()->Shutdown();
	TabBeingClosed->SetOnTabClosed( SDockTab::FOnTabClosedCallback() );
}

/*-----------------------------------------------------------------------------
	StatsMemoryDumpCommand
-----------------------------------------------------------------------------*/

void FProfilerModule::StatsMemoryDumpCommand( const TCHAR* Filename )
{
	//FRawStatsMemoryProfiler* Instance = FCreateStatsReader<FRawStatsMemoryProfiler>::ForRawStats( Filename );
	TUniquePtr<FRawStatsMemoryProfiler> Instance( FCreateStatsReader<FRawStatsMemoryProfiler>::ForRawStats( Filename ) );
	if (Instance)
	{
		Instance->ReadAndProcessAsynchronously();

		while (Instance->IsBusy())
		{
			FPlatformProcess::Sleep( 1.0f );

			UE_LOG( LogStats, Log, TEXT( "Async: Stage: %s / %3i%%" ), *Instance->GetProcessingStageAsString(), Instance->GetStageProgress() );
		}
		//NewReader->RequestStop();

		//Instance->

		// Frame-240 Frame-120 Frame-060
		TMap<FString, FCombinedAllocationInfo> FrameBegin_Exit;
		Instance->CompareSnapshots_FString( TEXT( "BeginSnapshot" ), TEXT( "EngineLoop.Exit" ), FrameBegin_Exit );
		Instance->DumpScopedAllocations( TEXT( "Begin_Exit" ), FrameBegin_Exit );

#if	UE_BUILD_DEBUG
		TMap<FString, FCombinedAllocationInfo> Frame060_120;
		Instance->CompareSnapshots_FString( TEXT( "Frame-060" ), TEXT( "Frame-120" ), Frame060_120 );
		Instance->DumpScopedAllocations( TEXT( "Frame060_120" ), Frame060_120 );

		TMap<FString, FCombinedAllocationInfo> Frame060_240;
		Instance->CompareSnapshots_FString( TEXT( "Frame-060" ), TEXT( "Frame-240" ), Frame060_240 );
		Instance->DumpScopedAllocations( TEXT( "Frame060_240" ), Frame060_240 );

		// Generate scoped tree view.
		{
			TMap<FName, FCombinedAllocationInfo> FrameBegin_Exit_FName;
			Instance->CompareSnapshots_FName( TEXT( "BeginSnapshot" ), TEXT( "EngineLoop.Exit" ), FrameBegin_Exit_FName );

			FNodeAllocationInfo Root;
			Root.EncodedCallstack = TEXT( "ThreadRoot" );
			Root.HumanReadableCallstack = TEXT( "ThreadRoot" );
			Instance->GenerateScopedTreeAllocations( FrameBegin_Exit_FName, Root );
		}


		{
			TMap<FName, FCombinedAllocationInfo> Frame060_240_FName;
			Instance->CompareSnapshots_FName( TEXT( "Frame-060" ), TEXT( "Frame-240" ), Frame060_240_FName );

			FNodeAllocationInfo Root;
			Root.EncodedCallstack = TEXT( "ThreadRoot" );
			Root.HumanReadableCallstack = TEXT( "ThreadRoot" );
			Instance->GenerateScopedTreeAllocations( Frame060_240_FName, Root );
		}
#endif // UE_BUILD_DEBUG
	}
}

/*-----------------------------------------------------------------------------
	FRawStatsMemoryProfiler
-----------------------------------------------------------------------------*/

FRawStatsMemoryProfiler* FProfilerModule::OpenRawStatsForMemoryProfiling( const TCHAR* Filename )
{
	FRawStatsMemoryProfiler* Instance = 0;// new FRawStatsMemoryProfiler;

	return Instance;
}



