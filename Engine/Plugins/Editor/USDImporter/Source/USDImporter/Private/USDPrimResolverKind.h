// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "USDPrimResolver.h"
#include "USDPrimResolverKind.generated.h"


/** Evaluates USD prims based on USD kind metadata */
UCLASS(transient, MinimalAPI)
class UUSDPrimResolverKind : public UUSDPrimResolver
{
	GENERATED_BODY()

public:
	virtual void FindActorsToSpawn(FUSDSceneImportContext& ImportContext, TArray<FActorSpawnData>& OutActorSpawnData) const override;

private:

	void FindActorsToSpawn_Recursive(FUSDSceneImportContext& ImportContext, class IUsdPrim* Prim, class IUsdPrim* ParentPrim, TArray<FActorSpawnData>& OutSpawnDatas) const;
};