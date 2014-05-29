// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/Commandlet.h"
#include "DumpBlueprintsInfoCommandlet.generated.h"

UCLASS()
class UDumpBlueprintsInfoCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

public:		
	// Begin UCommandlet Interface
	virtual int32 Main(FString const& Params) OVERRIDE;
	// End UCommandlet Interface
};
