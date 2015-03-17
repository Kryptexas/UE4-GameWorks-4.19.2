// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/Commandlet.h"
#include "UpdateGameProjectCommandlet.generated.h"

UCLASS()
class UUpdateGameProjectCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UUpdateGameProjectCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface
};
