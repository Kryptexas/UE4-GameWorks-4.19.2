// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/Commandlet.h"
#include "DumpBlueprintsInfoCommandlet.generated.h"

UCLASS()
class UDumpBlueprintsInfoCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UDumpBlueprintsInfoCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:		
	// Begin UCommandlet Interface
	virtual int32 Main(FString const& Params) override;
	// End UCommandlet Interface
};
