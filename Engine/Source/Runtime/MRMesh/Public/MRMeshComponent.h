// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "MRMeshComponent.generated.h"


class UMaterial;
class FBaseMeshReconstructorModule;
struct FDynamicMeshVertex;

DECLARE_STATS_GROUP(TEXT("MRMesh"), STATGROUP_MRMESH, STATCAT_Advanced);

DECLARE_DELEGATE(FOnProcessingComplete)
class IMRMesh
{
public:
	struct FSendBrickDataArgs
	{
		FIntVector BrickCoords;
		TArray<FDynamicMeshVertex>& Vertices;
		TArray<uint32>& Indices;
	};

	virtual void SendBrickData(FSendBrickDataArgs Args, const FOnProcessingComplete& OnProcessingComplete = FOnProcessingComplete()) = 0;
};



UCLASS(meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class MRMESH_API UMRMeshComponent : public UPrimitiveComponent, public IMRMesh
{
	friend class FMRMeshProxy;

	GENERATED_UCLASS_BODY()
private:
	//~ UPrimitiveComponent
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void BeginDestroy() override;
	//~ UPrimitiveComponent
	
	//~ IMRMesh
	virtual void SendBrickData(IMRMesh::FSendBrickDataArgs Args, const FOnProcessingComplete& OnProcessingComplete = FOnProcessingComplete()) override;
	//~ IMRMesh


private:
	void SendBrickData_Internal(IMRMesh::FSendBrickDataArgs Args, FOnProcessingComplete OnProcessingComplete);

	UPROPERTY(EditAnywhere, Category = Appearance)
	UMaterial* Material;

	FBaseMeshReconstructorModule* MeshReconstructor = nullptr;

};