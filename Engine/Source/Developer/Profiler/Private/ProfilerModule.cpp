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

	FRawStatsMemoryProfiler* OpenRawStatForMemoryProfiling( const TCHAR* Filename );


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

}

/*-----------------------------------------------------------------------------
	FRawStatsMemoryProfiler
-----------------------------------------------------------------------------*/

FRawStatsMemoryProfiler* FProfilerModule::OpenRawStatForMemoryProfiling( const TCHAR* Filename )
{
	FRawStatsMemoryProfiler* Instance = new FRawStatsMemoryProfiler;

	return Instance;
}



