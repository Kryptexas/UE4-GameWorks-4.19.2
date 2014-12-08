// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealHeaderTool.h"
#include "StringUtils.h"

FString GetClassNameWithPrefixRemoved(const FString InClassName)
{
	const FString ClassPrefix = GetClassPrefix( InClassName );
	if( !ClassPrefix.IsEmpty() )
	{	
		return InClassName.Right(InClassName.Len() - ClassPrefix.Len());
	}
	return FString();
}

FString GetClassNameWithoutPrefix( const FString& InClassNameOrFilename )
{
	// Check for header names (they don't come with a full path so we only search for the first dot)
	const int32 DotIndex = InClassNameOrFilename.Find(TEXT("."));
	if (DotIndex == INDEX_NONE)
	{
		const FString ClassPrefix = GetClassPrefix( InClassNameOrFilename );
		return InClassNameOrFilename.Right(InClassNameOrFilename.Len() - ClassPrefix.Len());
	}
	else
	{
		return InClassNameOrFilename.Mid(0, DotIndex);
	}
}

FString GetClassPrefix( const FString InClassName )
{
	bool bIsLabledDeprecated;
	return GetClassPrefix(InClassName, /*out*/ bIsLabledDeprecated);
}

FString GetClassPrefix( const FString InClassName, bool& bIsLabeledDeprecated )
{
	FString ClassPrefix = InClassName.Left(1);

	bIsLabeledDeprecated = false;
	// First make sure the class name starts with a valid prefix
	if( ClassPrefix == TEXT("I") ||
		ClassPrefix == TEXT("A") || 
		ClassPrefix == TEXT("U") )
	{
		// If it is a class prefix, check for deprecated class prefix
		if( InClassName.Mid(1, 11) == TEXT("DEPRECATED_") )
		{
			bIsLabeledDeprecated = true;
			return InClassName.Left(12);
		}
	}
	// Otherwise check for struct prefix. If it's not a class or struct prefix, it's invalid
	else if( ClassPrefix != TEXT("F") && ClassPrefix != TEXT("T") )
	{
		ClassPrefix = TEXT("");
	}
	return ClassPrefix;
}
