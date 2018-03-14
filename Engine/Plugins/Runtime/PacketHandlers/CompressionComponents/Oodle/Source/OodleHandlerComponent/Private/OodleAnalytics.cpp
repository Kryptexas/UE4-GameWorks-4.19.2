// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OodleAnalytics.h"
#include "OodleHandlerComponent.h"
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"

class FOodleAnalyticsImpl
	: public FOodleAnalytics
{
public:

	virtual ~FOodleAnalyticsImpl() { }

	virtual void SetProvider(TSharedPtr<IAnalyticsProvider> InAnalyticsProvider) override
	{
		AnalyticsProvider = InAnalyticsProvider;
	}

	virtual void FireEvent_OodleAnalytics(float InTotalSavings, float OutTotalSavings) const override
	{
		if (AnalyticsProvider.IsValid())
		{
			static const FString EventName = TEXT("Oodle.Stats");

			TArray<FAnalyticsEventAttribute> Attributes;
			Attributes.Add(FAnalyticsEventAttribute(TEXT("InTotalSavings"), InTotalSavings));
			Attributes.Add(FAnalyticsEventAttribute(TEXT("OutTotalSavings"), OutTotalSavings));
			AnalyticsProvider->RecordEvent(EventName, Attributes);
		}
	}

private: // functions

	FOodleAnalyticsImpl()
	{}

private: // data

	// cached analytics provider for pushing events
	TSharedPtr<IAnalyticsProvider> AnalyticsProvider;

	friend FOodleAnalyticsFactory;
};

TSharedRef< FOodleAnalytics > FOodleAnalyticsFactory::Create()
{
	TSharedRef< FOodleAnalytics > Instance = MakeShareable(new FOodleAnalyticsImpl());
	return Instance;
}
