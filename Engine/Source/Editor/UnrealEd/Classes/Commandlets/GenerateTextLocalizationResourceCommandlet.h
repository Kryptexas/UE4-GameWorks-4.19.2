// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "Commandlets/Commandlet.h"
#include "GenerateTextLocalizationResourceCommandlet.generated.h"

/**
 *	UGenerateTextLocalizationResourceCommandlet: Localization commandlet that generates a table of FText keys to localized string values.
 */

UCLASS()
class UGenerateTextLocalizationResourceCommandlet : public UGatherTextCommandletBase
{
	GENERATED_BODY()
public:
	UGenerateTextLocalizationResourceCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface
};
