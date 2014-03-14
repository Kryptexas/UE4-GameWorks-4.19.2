// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ProfilerModule.cpp: Implements the FProfilerModule class.
=============================================================================*/

#include "ProfilerPrivatePCH.h"

/**
 * Implements the FProfilerModule module.
 */
class FProfilerModule
	: public IProfilerModule
{
public:
	virtual TSharedRef<SWidget> CreateProfilerWindow( const ISessionManagerRef& InSessionManager, const TSharedRef<SDockTab>& ConstructUnderMajorTab ) OVERRIDE
	{
		ProfilerManager = FProfilerManager::Initialize( InSessionManager );
		TSharedRef<SProfilerWindow> ProfilerWindow = SNew(SProfilerWindow, InSessionManager);
		FProfilerManager::Get()->AssignProfilerWindow( ProfilerWindow );
		// Register OnTabClosed to handle profiler manager shutdown.
		ConstructUnderMajorTab->SetOnTabClosed( SDockTab::FOnTabClosedCallback::CreateRaw(this, &FProfilerModule::Shutdown) );

		return ProfilerWindow;
	}

	virtual void StartupModule() OVERRIDE
	{}

	virtual void ShutdownModule() OVERRIDE
	{
		if( FProfilerManager::Get().IsValid() )
		{
			FProfilerManager::Get()->Shutdown();
		}
	}

	virtual bool SupportsDynamicReloading() OVERRIDE
	{
		return false;
	}

	virtual TWeakPtr<class IProfilerManager> GetProfilerManager() OVERRIDE
	{
		return ProfilerManager;
	}

protected:
	/** Shutdowns the profiler manager. */
	void Shutdown( TSharedRef<SDockTab> TabBeingClosed )
	{
		FProfilerManager::Get()->Shutdown();
		TabBeingClosed->SetOnTabClosed( SDockTab::FOnTabClosedCallback() );
	}

	/** A weak pointer to the global instance of the profiler manager. Will last as long as profiler window is visible. */
	TWeakPtr<class IProfilerManager> ProfilerManager;
};

IMPLEMENT_MODULE(FProfilerModule, Profiler);