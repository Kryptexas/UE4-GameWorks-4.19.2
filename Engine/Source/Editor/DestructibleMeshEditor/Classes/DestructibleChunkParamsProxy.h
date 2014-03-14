// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "Runtime/Engine/Classes/Engine/DestructibleFractureSettings.h"
#include "DestructibleChunkParamsProxy.generated.h"


UCLASS()
class UDestructibleChunkParamsProxy : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UDestructibleMesh* DestructibleMesh;

	UPROPERTY()
	int32 ChunkIndex;

	UPROPERTY(EditAnywhere, Category=Chunks)
	FDestructibleChunkParameters ChunkParams;

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface
#endif
};