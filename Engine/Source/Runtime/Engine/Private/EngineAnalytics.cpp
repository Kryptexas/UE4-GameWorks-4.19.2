// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Analytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"


bool FEngineAnalytics::bIsInitialized;
TSharedPtr<IAnalyticsProvider> FEngineAnalytics::Analytics;

/**
 * Engine analytics config log to initialize the engine analytics provider.
 * External code should bind this delegate if engine analytics are desired,
 * preferably in private code that won't be redistributed.
 */
FAnalytics::FProviderConfigurationDelegate& GetEngineAnalyticsConfigDelegate()
{
	static FAnalytics::FProviderConfigurationDelegate Delegate;
	return Delegate;
}

FSimpleDelegate& OnEngineAnalyticsStartupDelegate()
{
	static FSimpleDelegate Delegate;
	return Delegate;
}


/**
 * On-demand construction of the singleton. 
 */
IAnalyticsProvider& FEngineAnalytics::GetProvider()
{
	checkf(bIsInitialized && Analytics.IsValid(), TEXT("FEngineAnalytics::GetProvider called outside of Initialize/Shutdown."));
	return *Analytics.Get();
}

void FEngineAnalytics::Initialize()
{
	checkf(!bIsInitialized, TEXT("FEngineAnalytics::Initialize called more than once."));

	// Never use analytics when running a commandlet tool
	const bool bShouldInitAnalytics = !IsRunningCommandlet();
	if( bShouldInitAnalytics )
	{
		// Connect the engine analytics provider (if there is a configuration delegate installed)
		if (GetEngineAnalyticsConfigDelegate().IsBound())
		{
		
			Analytics = FAnalytics::Get().CreateAnalyticsProvider(
				FName(*GetEngineAnalyticsConfigDelegate().Execute(TEXT("ProviderModuleName"), true)), 
				GetEngineAnalyticsConfigDelegate());
			if (Analytics.IsValid())
			{
				Analytics->SetUserID(FPlatformMisc::GetUniqueDeviceId());
				Analytics->StartSession();
			}
		}
	}
	bIsInitialized = true;

	if( bIsInitialized )
	{
		// Allow the analytics configuration to do whatever it wants after starting up
		OnEngineAnalyticsStartupDelegate().ExecuteIfBound();
	}
}


void FEngineAnalytics::Shutdown()
{
	checkf(bIsInitialized, TEXT("FEngineAnalytics::Shutdown called outside of Initialize."));
	Analytics.Reset();
	bIsInitialized = false;
}
