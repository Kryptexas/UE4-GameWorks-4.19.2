// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IAnalyticsProvider.h"

#define FACTORY(ReturnType, Type, ...) \
class Type##Factory \
{ \
public: \
	static ReturnType Create(__VA_ARGS__); \
}; 

class FOodleAnalytics
{
public:

	virtual ~FOodleAnalytics() { }

	/**
	* Update provider to use for capturing events
	*/
	virtual void SetProvider(TSharedPtr<IAnalyticsProvider> InAnalyticsProvider) = 0;

	/*
	* @EventName Oodle.Stats
	*
	* @Trigger Fires event for percentage compression by Oodle
	*
	* @EventParam InTotalSavings Percentage savings in traffic sent by a client
	* @EventParam OutTotalSavings Percentage savings in traffic sent by the server
	*
	* @Comments
	*
	*/
	virtual void FireEvent_OodleAnalytics(float InTotalSavings, float OutTotalSavings) const = 0;
};

FACTORY(TSharedRef<class FOodleAnalytics>, FOodleAnalytics);
