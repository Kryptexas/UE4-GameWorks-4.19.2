// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ReimportFbxAnimSequenceFactory
//=============================================================================

#pragma once
#include "ReimportFbxAnimSequenceFactory.generated.h"

UCLASS(collapsecategories)
class UReimportFbxAnimSequenceFactory : public UFbxFactory, public FReimportHandler
{
	GENERATED_BODY()
public:
	UReimportFbxAnimSequenceFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin FReimportHandler interface
	virtual bool CanReimport( UObject* Obj, TArray<FString>& OutFilenames ) override;
	virtual void SetReimportPaths( UObject* Obj, const TArray<FString>& NewReimportPaths ) override;
	virtual EReimportResult::Type Reimport( UObject* Obj ) override;
	virtual int32 GetPriority() const override;
	// End FReimportHandler interface
};



