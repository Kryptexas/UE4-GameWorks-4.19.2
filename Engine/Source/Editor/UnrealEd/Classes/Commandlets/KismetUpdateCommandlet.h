// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * This commandlet refreshes and recompiles Kismet blueprints.
 */

#pragma once
#include "KismetUpdateCommandlet.generated.h"

UCLASS()
class UKismetUpdateCommandlet : public UResavePackagesCommandlet
{
	GENERATED_UCLASS_BODY()

protected:
	TSet<FString> AlreadyCompiledFullPaths;
protected:
	// UResavePackagesCommandlet interface
	virtual int32 InitializeResaveParameters(const TArray<FString>& Tokens, const TArray<FString>& Switches, TArray<FString>& MapPathNames) OVERRIDE;
	virtual void PerformAdditionalOperations(class UObject* Object, bool& bSavePackage) OVERRIDE;
	virtual FString GetChangelistDescription() const OVERRIDE;
	// End of UResavePackagesCommandlet interface

	void CompileOneBlueprint(UBlueprint* Blueprint);
};
