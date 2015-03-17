// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ReimportCurveFactory.generated.h"

UCLASS()
class UReimportCurveFactory : public UCSVImportFactory, public FReimportHandler
{
	GENERATED_BODY()
public:
	UReimportCurveFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin FReimportHandler interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) override;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) override;
	virtual EReimportResult::Type Reimport( UObject* Obj ) override;
	virtual int32 GetPriority() const override;
	// End FReimportHandler interface
};



