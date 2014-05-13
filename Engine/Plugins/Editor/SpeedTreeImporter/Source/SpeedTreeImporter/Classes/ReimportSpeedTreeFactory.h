// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ReimportSpeedTreeFactory
//=============================================================================

#pragma once
#include "SpeedTreeImportFactory.h"
#include "ReimportSpeedTreeFactory.generated.h"

UCLASS(collapsecategories)
class UReimportSpeedTreeFactory : public USpeedTreeImportFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

	// Begin FReimportHandler interface
#if WITH_SPEEDTREE
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) OVERRIDE;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) OVERRIDE;
	virtual EReimportResult::Type Reimport(UObject* Obj) OVERRIDE;
#endif
	// End FReimportHandler interface
};



