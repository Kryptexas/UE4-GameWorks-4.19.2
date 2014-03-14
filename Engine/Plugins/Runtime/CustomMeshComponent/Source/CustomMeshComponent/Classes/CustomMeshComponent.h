// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CustomMeshComponent.generated.h"

USTRUCT(BlueprintType)
struct FCustomMeshTriangle
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Triangle)
	FVector Vertex0;

	UPROPERTY(EditAnywhere, Category=Triangle)
	FVector Vertex1;

	UPROPERTY(EditAnywhere, Category=Triangle)
	FVector Vertex2;
};

/** Component that allows you to specify custom triangle mesh geometry */
UCLASS(hidecategories=(Object,LOD, Physics, Collision), editinlinenew, meta=(BlueprintSpawnableComponent), ClassGroup=Rendering)
class UCustomMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** Set the geometry to use on this triangle mesh */
	UFUNCTION(BlueprintCallable, Category="Components|CustomMesh")
	bool SetCustomMeshTriangles(const TArray<FCustomMeshTriangle>& Triangles);

private:

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin UMeshComponent interface.
	virtual int32 GetNumMaterials() const OVERRIDE;
	// End UMeshComponent interface.

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// Begin USceneComponent interface.

	/** */
	TArray<FCustomMeshTriangle> CustomMeshTris;

	friend class FCustomMeshSceneProxy;
};


