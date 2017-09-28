// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FlexMigrateContentCommandlet.generated.h"

UCLASS()
class UFlexMigrateContentCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()

	virtual int32 Main(const FString& Params) override;

private:
	bool MigrateStaticMesh(class UStaticMesh* StaticMesh);
};
