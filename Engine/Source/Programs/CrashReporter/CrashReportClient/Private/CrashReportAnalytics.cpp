// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "CrashReportAnalytics.h"
#include "EngineBuildSettings.h"
#include "Runtime/Analytics/AnalyticsET/Public/AnalyticsET.h"

// WARNING! Copied from EngineAnalytics.cpp

bool FCrashReportAnalytics::bIsInitialized;
TSharedPtr<IAnalyticsProvider> FCrashReportAnalytics::Analytics;

/**
 * Engine analytics config log to initialize the engine analytics provider.
 * External code should bind this delegate if engine analytics are desired,
 * preferably in private code that won't be redistributed.
 */
FAnalyticsET::Config& GetCrashReportAnalyticsOverrideConfig()
{
	static FAnalyticsET::Config Config;
	return Config;
}


/**
 * On-demand construction of the singleton. 
 */
IAnalyticsProvider& FCrashReportAnalytics::GetProvider()
{
	checkf(bIsInitialized && Analytics.IsValid(), TEXT("FCrashReportAnalytics::GetProvider called outside of Initialize/Shutdown."));
	return *Analytics.Get();
}
 
void FCrashReportAnalytics::Initialize()
{
	checkf(!bIsInitialized, TEXT("FCrashReportAnalytics::Initialize called more than once."));

	if (!GetCrashReportAnalyticsOverrideConfig().APIServerET.IsEmpty())
	{
		// Connect the engine analytics provider (if there is a configuration delegate installed)
		Analytics = FAnalyticsET::Get().CreateAnalyticsProvider(GetCrashReportAnalyticsOverrideConfig());
		if( Analytics.IsValid() )
		{
			Analytics->SetUserID(FString::Printf(TEXT("%s|%s|%s"), *FPlatformMisc::GetMachineId().ToString(EGuidFormats::Digits).ToLower(), *FPlatformMisc::GetEpicAccountId(), *FPlatformMisc::GetOperatingSystemId()));
			Analytics->StartSession();
		}
	}
	bIsInitialized = true;
}


void FCrashReportAnalytics::Shutdown()
{
	checkf(bIsInitialized, TEXT("FCrashReportAnalytics::Shutdown called outside of Initialize."));
	Analytics.Reset();
	bIsInitialized = false;
}
