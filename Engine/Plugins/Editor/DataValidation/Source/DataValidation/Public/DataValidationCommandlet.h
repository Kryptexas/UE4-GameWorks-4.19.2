// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Commandlets/Commandlet.h"
#include "UObject/Interface.h"
#include "DataValidationCommandlet.generated.h"

UCLASS(CustomConstructor)
class DATAVALIDATION_API UDataValidationCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

public:
	UDataValidationCommandlet(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		LogToConsole = false;
	}

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface

	// do the validation without creating a commandlet
	static bool ValidateData();
};
