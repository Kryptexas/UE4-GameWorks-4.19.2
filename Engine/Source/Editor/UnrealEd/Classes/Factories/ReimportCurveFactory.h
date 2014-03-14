// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ReimportCurveFactory.generated.h"

UCLASS()
class UReimportCurveFactory : public UCSVImportFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()


	// Begin FReimportHandler interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) OVERRIDE;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) OVERRIDE;
	virtual EReimportResult::Type Reimport( UObject* Obj ) OVERRIDE;
	// End FReimportHandler interface
};



