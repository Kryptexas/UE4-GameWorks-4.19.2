// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "GenerateTextLocalizationResourceCommandlet.generated.h"

/**
 *	UGenerateTextLocalizationResourceCommandlet: Localization commandlet that generates a table of FText keys to localized string values.
 */

UCLASS()
class UGenerateTextLocalizationResourceCommandlet : public UGatherTextCommandletBase
{
	GENERATED_UCLASS_BODY()

public:
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface
};