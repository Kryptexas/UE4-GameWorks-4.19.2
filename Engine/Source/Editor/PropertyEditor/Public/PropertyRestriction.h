// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class PROPERTYEDITOR_API FPropertyRestriction
{
public:
	
	FPropertyRestriction(const FText& InReason)
		:Reason(InReason)
	{
	}

	FText GetReason()const;
	bool IsValueRestricted(const FString& InValue)const;
	void AddValue(const FString& InValue);

private:

	TArray<FString> Values;
	FText Reason;
};
