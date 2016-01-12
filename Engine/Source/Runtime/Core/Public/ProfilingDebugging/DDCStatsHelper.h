// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class FDDCScopeStatHelper
{
private:
	FName TransactionGuid;
	double StartTime;
	bool bHasParent;
public:
	CORE_API FDDCScopeStatHelper(const TCHAR* CacheKey, const FName& FunctionName);

	CORE_API ~FDDCScopeStatHelper();

	CORE_API void AddTag(const FName& Tag, const FString& Value);

	CORE_API void AddTag(const FName& Tag, const float Value);

	CORE_API void AddTag(const FName& Tag, const int32 Value);

	CORE_API void AddTag(const FName& Tag, const bool Value);

	CORE_API bool HasParent() const { return bHasParent; }
};

