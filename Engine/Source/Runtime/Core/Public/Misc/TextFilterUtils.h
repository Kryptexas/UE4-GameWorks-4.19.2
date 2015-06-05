// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

/** Defines the comparison operators that can be used for a complex (key->value) comparison */
enum class ETextFilterComparisonOperation : uint8
{
	Equal,
	NotEqual,
	Less,
	LessOrEqual,
	Greater,
	GreaterOrEqual,
};

/** Defines the different ways that a string can be compared while evaluating the expression */
enum class ETextFilterTextComparisonMode : uint8
{
	Exact,
	Partial,
	StartsWith,
	EndsWith,
};

namespace TextFilterUtils
{
	/** Utility function to perform a basic string test for the given values */
	CORE_API bool TestBasicStringExpression(const FString& InValue1, const FString& InValue2, const ETextFilterTextComparisonMode InTextComparisonMode);

	/** Utility function to perform a complex expression test for the given values */
	CORE_API bool TestComplexExpression(const FString& InValue1, const FString& InValue2, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode);
}
