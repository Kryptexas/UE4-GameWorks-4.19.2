// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ReimportSoundFactory
//=============================================================================

#pragma once
#include "ReimportSoundFactory.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UReimportSoundFactory : public USoundFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()


	// Begin FReimportHandler interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) OVERRIDE;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) OVERRIDE;
	virtual EReimportResult::Type Reimport( UObject* Obj ) OVERRIDE;
	// End FReimportHandler interface
};



