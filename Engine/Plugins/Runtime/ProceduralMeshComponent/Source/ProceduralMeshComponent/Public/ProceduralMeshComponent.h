// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/MeshComponent.h"
#include "ProceduralMeshComponent.generated.h"

USTRUCT(BlueprintType)
struct FProcMeshTangent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	FVector TangentX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	bool bFlipTangentY;

	FProcMeshTangent()
	: TangentX(1.f, 0.f, 0.f)
	, bFlipTangentY(false)
	{}

	FProcMeshTangent(float X, float Y, float Z)
	: TangentX(X, Y, Z)
	, bFlipTangentY(false)
	{}

	FProcMeshTangent(FVector InTangentX, bool bInFlipTangentY)
	: TangentX(InTangentX)
	, bFlipTangentY(bInFlipTangentY)
	{}
};


USTRUCT()
struct FProcMeshVertex
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FVector Normal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FProcMeshTangent Tangent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FVector2D UV0;


	FProcMeshVertex()
	: Position(0.f, 0.f, 0.f)
	, Normal(0.f, 0.f, 1.f)
	, Tangent(FVector(1.f, 0.f, 0.f), false)
	, Color(255, 255, 255)
	, UV0(0.f, 0.f)
	{}
};

/** Component that allows you to specify custom triangle mesh geometry */
UCLASS(hidecategories=(Object,LOD), meta=(BlueprintSpawnableComponent), ClassGroup=Rendering)
class PROCEDURALMESHCOMPONENT_API UProceduralMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** Add triangle to the geometry to use on this triangle mesh. Must call BuildMesh before geometry can be seen. */
	UFUNCTION(BlueprintCallable, Category = "Components|ProceduralMesh", meta=(AutoCreateRefTerm = "Normals,UV0,VertexColors,Tangents" ))
	void CreateMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents);


private:

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent interface.

	// Begin UMeshComponent interface.
	virtual int32 GetNumMaterials() const override;
	// End UMeshComponent interface.

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// Begin USceneComponent interface.

	// Members below are updated when BuildMesh is called.

	/** Vertex buffer for proc mesh */
	TArray<FProcMeshVertex> ProcVertexBuffer;
	/** Index buffer for proc mesh */
	TArray<int32> ProcIndexBuffer;
	/** Local bounds of mesh */
	FBoxSphereBounds LocalBounds;

	friend class FProceduralMeshSceneProxy;
};


