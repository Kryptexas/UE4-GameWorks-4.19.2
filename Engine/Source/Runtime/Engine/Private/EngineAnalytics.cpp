// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Analytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"


bool FEngineAnalytics::bIsInitialized;
TSharedPtr<IAnalyticsProvider> FEngineAnalytics::Analytics;

#define DEBUG_ANALYTICS 0

#if DEBUG_ANALYTICS

DEFINE_LOG_CATEGORY_STATIC(LogDebugAnalytics, Log, All);

namespace DebugAnalyticsProviderDefs
{
	const static int32 MaxAttributeCount = 40;
}

/**
 * Debug analytics provider for debugging analytics events
 * without the need to connect to a real provider.
 */
class FDebugAnalyticsProvider : public IAnalyticsProvider
{
public:
	virtual bool StartSession(const TArray<FAnalyticsEventAttribute>& Attributes)
	{
		UE_LOG(LogDebugAnalytics, Log, TEXT("FDebugAnalyticsProvider: StartSession..."));
		OutputAttributesToLog(Attributes);
		return true;
	}

	virtual void EndSession()
	{
		UE_LOG(LogDebugAnalytics, Log, TEXT("FDebugAnalyticsProvider: EndSession"));
	}

	virtual FString GetSessionID() const
	{
		return SessionID;
	}

	virtual bool SetSessionID(const FString& InSessionID)
	{
		UE_LOG(LogDebugAnalytics, Log, TEXT("FDebugAnalyticsProvider: SetSessionID SessionID=%s"), *InSessionID);
		SessionID = InSessionID;
		return true;
	}

	virtual void FlushEvents()
	{
	}

	virtual void SetUserID(const FString& InUserID)
	{
		UE_LOG(LogDebugAnalytics, Log, TEXT("FDebugAnalyticsProvider: SetUserID UserID=%s"), *InUserID);
		UserID = InUserID;
	}

	virtual FString GetUserID() const
	{
		return UserID;
	}

	virtual void RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes)
	{
		if (Attributes.Num() > DebugAnalyticsProviderDefs::MaxAttributeCount)
		{
			UE_LOG(LogDebugAnalytics, Warning, TEXT("FDebugAnalyticsProvider: Event %s has too many attributes (%d)"), *EventName, Attributes.Num());
		}

		UE_LOG(LogDebugAnalytics, Log, TEXT("FDebugAnalyticsProvider: RecordEvent Name=%s"), *EventName);
		OutputAttributesToLog(Attributes);
	}

private:
	void OutputAttributesToLog(const TArray<FAnalyticsEventAttribute>& Attributes)
	{
		for (auto& Attrib : Attributes)
		{
			UE_LOG(LogDebugAnalytics, Log, TEXT("AnalyticsAttrib Name=%s, Value=%s"), *Attrib.AttrName, *Attrib.AttrValue);
		}
	}

	FString SessionID;
	FString UserID;
};

#endif // DEBUG_ANALYTICS

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
#if DEBUG_ANALYTICS
		// Dummy analytics provider just logs events locally
		Analytics = MakeShareable(new FDebugAnalyticsProvider());
		Analytics->SetUserID(FPlatformMisc::GetUniqueDeviceId());
		Analytics->StartSession();
#else
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
#endif
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
