// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/Commandlet.h"
#include "GenerateBlueprintAPICommandlet.generated.h"

UCLASS()
class UGenerateBlueprintAPICommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UGenerateBlueprintAPICommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:		
	// Begin UCommandlet Interface
	virtual int32 Main(FString const& Params) override;
	// End UCommandlet Interface
};
