// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/ResavePackagesCommandlet.h"
#include "RecompileBlueprintsCommandlet.generated.h"

UCLASS()
class URecompileBlueprintsCommandlet : public UResavePackagesCommandlet
{
    GENERATED_UCLASS_BODY()

public:		
	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface

	// Begin UResavePackagesCommandlet Interface
	virtual bool ShouldSkipPackage(const FString& Filename) override;
	virtual bool PerformPreloadOperations( ULinkerLoad* PackageLinker, bool& bSavePackage ) override;
	virtual void PerformAdditionalOperations( class UObject* Object, bool& bSavePackage ) override;
	virtual int32 InitializeResaveParameters( const TArray<FString>& Tokens, const TArray<FString>& Switches, TArray<FString>& MapPathNames ) override;
	// End UResavePackagesCommandlet Interface
};
