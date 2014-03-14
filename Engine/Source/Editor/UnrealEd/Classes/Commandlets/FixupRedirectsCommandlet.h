// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FixupRedirectsCommandlet.generated.h"

UCLASS()
class UFixupRedirectsCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	// Begin UCommandlet Interface
	virtual void CreateCustomEngine(const FString& Params) OVERRIDE;
	virtual int32 Main(const FString& Params) OVERRIDE;
	// End UCommandlet Interface
};


