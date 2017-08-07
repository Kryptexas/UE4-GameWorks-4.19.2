// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnalyticsConversion.h"

/**
 * Struct to hold key/value pairs that will be sent as attributes along with analytics events.
 * All values are actually strings, but we provide a convenient constructor that relies on ToStringForAnalytics() to 
 * convert common types. 
 */
struct FAnalyticsEventAttribute
{
	FString AttrName;
	FString AttrValue;
	/** Default ctor since we declare a custom ctor. */
	FAnalyticsEventAttribute()
	{}

	/**
	 * Helper constructor to make an attribute from a name/value pair by forwarding through Lex::ToString and AnalyticsConversion::ToString.
	 * 
	 * @param InName Name of the attribute. Will be converted to a string via forwarding to Lex::ToString
	 * @param InValue Value of the attribute. Will be converted to a string via forwarding to AnalyticsConversion::ToString (same as Lex but with basic support for arrays and maps)
	 */
	template <typename NameType, typename ValueType>
	FAnalyticsEventAttribute(NameType&& InName, ValueType&& InValue)
		: AttrName(Lex::ToString(Forward<NameType>(InName)))
		, AttrValue(AnalyticsConversion::ToString(Forward<ValueType>(InValue)))
	{}
};

/** Helper functions for MakeAnalyticsEventAttributeArray. */
namespace ImplMakeAnalyticsEventAttributeArray
{
	/** Recursion terminator. Empty list. */
	template <typename Allocator>
	inline void MakeArray(TArray<FAnalyticsEventAttribute, Allocator>& Attrs)
	{
	}

	/** Recursion terminator. Convert the key/value pair to analytics strings. */
	template <typename Allocator, typename KeyType, typename ValueType>
	inline void MakeArray(TArray<FAnalyticsEventAttribute, Allocator>& Attrs, KeyType&& Key, ValueType&& Value)
	{
		Attrs.Emplace(Forward<KeyType>(Key), Forward<ValueType>(Value));
	}

	/** recursively add the arguments to the array. */
	template <typename Allocator, typename KeyType, typename ValueType, typename...ArgTypes>
	inline void MakeArray(TArray<FAnalyticsEventAttribute, Allocator>& Attrs, KeyType&& Key, ValueType&& Value, ArgTypes&&...Args)
	{
		// pop off the top two args and recursively apply the rest.
		Attrs.Emplace(Forward<KeyType>(Key), Forward<ValueType>(Value));
		MakeArray(Attrs, Forward<ArgTypes>(Args)...);
	}
}

/** Helper to create an array of attributes using a single expression. Reserves the necessary space in advance. There must be an even number of arguments, one for each key/value pair. */
template <typename Allocator = FDefaultAllocator, typename...ArgTypes>
inline TArray<FAnalyticsEventAttribute, Allocator> MakeAnalyticsEventAttributeArray(ArgTypes&&...Args)
{
	static_assert(sizeof...(Args) % 2 == 0, "Must pass an even number of arguments.");
	TArray<FAnalyticsEventAttribute, Allocator> Attrs;
	Attrs.Empty(sizeof...(Args) / 2);
	ImplMakeAnalyticsEventAttributeArray::MakeArray(Attrs, Forward<ArgTypes>(Args)...);
	return Attrs;
}

/** Helper to append to an array of attributes using a single expression. Reserves the necessary space in advance. There must be an even number of arguments, one for each key/value pair. */
template <typename Allocator = FDefaultAllocator, typename...ArgTypes>
inline TArray<FAnalyticsEventAttribute, Allocator>& AppendAnalyticsEventAttributeArray(TArray<FAnalyticsEventAttribute, Allocator>& Attrs, ArgTypes&&...Args)
{
	static_assert(sizeof...(Args) % 2 == 0, "Must pass an even number of arguments.");
	Attrs.Reserve(Attrs.Num() + (sizeof...(Args) / 2));
	ImplMakeAnalyticsEventAttributeArray::MakeArray(Attrs, Forward<ArgTypes>(Args)...);
	return Attrs;
}
