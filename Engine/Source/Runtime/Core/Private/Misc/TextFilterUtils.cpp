// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "TextFilterUtils.h"

bool TextFilterUtils::TestBasicStringExpression(const FString& InValue1, const FString& InValue2, const ETextFilterTextComparisonMode InTextComparisonMode)
{
	switch(InTextComparisonMode)
	{
	case ETextFilterTextComparisonMode::Exact:
		return InValue1.Equals(InValue2, ESearchCase::IgnoreCase);
	case ETextFilterTextComparisonMode::Partial:
		return InValue1.Contains(InValue2, ESearchCase::IgnoreCase);
	case ETextFilterTextComparisonMode::StartsWith:
		return InValue1.StartsWith(InValue2, ESearchCase::IgnoreCase);
	case ETextFilterTextComparisonMode::EndsWith:
		return InValue1.EndsWith(InValue2, ESearchCase::IgnoreCase);
	default:
		break;
	}
	return false;
}

bool TextFilterUtils::TestComplexExpression(const FString& InValue1, const FString& InValue2, const ETextFilterComparisonOperation InComparisonOperation, const ETextFilterTextComparisonMode InTextComparisonMode)
{
	const bool bNumericComparison = InValue1.IsNumeric() && InValue2.IsNumeric();

	int32 ComparisonSign = 0;
	if (bNumericComparison)
	{
		const double Difference = FCString::Atod(*InValue1) - FCString::Atod(*InValue2);
		ComparisonSign = (int32)FMath::Sign(Difference);
	}
	else if (InTextComparisonMode == ETextFilterTextComparisonMode::Exact)
	{
		ComparisonSign = InValue1.Compare(InValue2, ESearchCase::IgnoreCase);
	}
	else
	{
		ComparisonSign = (InValue1.Contains(InValue2, ESearchCase::IgnoreCase)) ? 0 : 1;
	}

	bool bComparisonPassed = false;
	switch (InComparisonOperation)
	{
	case ETextFilterComparisonOperation::Equal:
		bComparisonPassed = ComparisonSign == 0;
		break;
	case ETextFilterComparisonOperation::NotEqual:
		bComparisonPassed = ComparisonSign != 0;
		break;
	case ETextFilterComparisonOperation::Less:
		bComparisonPassed = ComparisonSign < 0;
		break;
	case ETextFilterComparisonOperation::LessOrEqual:
		bComparisonPassed = ComparisonSign <= 0;
		break;
	case ETextFilterComparisonOperation::Greater:
		bComparisonPassed = ComparisonSign > 0;
		break;
	case ETextFilterComparisonOperation::GreaterOrEqual:
		bComparisonPassed = ComparisonSign >= 0;
		break;
	default:
		check(false);
	};

	return bComparisonPassed;
}
