// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Commandlets/Commandlet.h"
#include "ListStaticMeshesImportedFromSpeedTreesCommandlet.generated.h"

UCLASS()
class UListStaticMeshesImportedFromSpeedTreesCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	UListStaticMeshesImportedFromSpeedTreesCommandlet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	// End UCommandlet Interface
};
