// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ReimportSoundSurroundFactory
//=============================================================================

#pragma once
#include "ReimportSoundSurroundFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UReimportSoundSurroundFactory : public USoundSurroundFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FString> ReimportPaths;

	// Begin FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) OVERRIDE;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) OVERRIDE;
	virtual EReimportResult::Type Reimport(UObject* Obj) OVERRIDE;
	// End FReimportHandler interface
};
