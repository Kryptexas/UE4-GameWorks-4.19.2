// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"

/** 
 * Struct to hold key/value pairs that will be sent as attributes along with analytics events.
 * All values are actually strings, but we provide convenient conversion functions for most basic types. 
 */
struct FAnalyticsEventAttribute
{
	FString AttrName;
	FString AttrValue;

	FAnalyticsEventAttribute(const FString& InName, const FString& InValue)
		:AttrName(InName)
		,AttrValue(InValue)
	{
	}

	FAnalyticsEventAttribute(const FString& InName, const TCHAR* InValue)
		:AttrName(InName)
		,AttrValue(InValue)
	{
	}

	FAnalyticsEventAttribute(const FString& InName, bool InValue)
		:AttrName(InName)
		,AttrValue(InValue ? TEXT("true") : TEXT("false"))
	{
	}

	// Allow any type that we have a valid format specifier for (disallowing implicit conversions).
	template <typename T>
	FAnalyticsEventAttribute(const FString& InName, T InValue)
		:AttrName(InName)
		,AttrValue(TTypeToString<T>::ToString(InValue))
	{
	}
};
