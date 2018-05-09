// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameWorks/FlexPluginCommon.h"
#include "FlexAsset.generated.h"


// Flex extensions types
struct FlexSolver;
struct FlexExtContainer;
struct NvFlexExtAsset;
struct NvFlexExtInstance;

class UFlexContainer;

class UStaticMesh;


/* A Flex asset contains the particle and constraint data for a shape, such as cloth, rigid body or inflatable, an asset 
   is added to a container by spawning through a particle system or Flex actor. */
UCLASS(Abstract, config = Engine, editinlinenew)
class FLEX_API UFlexAsset : public UObject
{
public:

	GENERATED_UCLASS_BODY()

	/** The simulation container to spawn any flex data contained in the static mesh into */
	UPROPERTY(EditAnywhere, Category = Flex, meta = (ToolTip = "If the static mesh has Flex data then it will be spawned into this simulation container."))
	UFlexContainer* ContainerTemplate;

	/** The phase to assign to particles spawned for this mesh */
	UPROPERTY(EditAnywhere, Category = Flex)
	FFlexPhase Phase;

	/** If true then the particles will be attached to any overlapping shapes on spawn*/
	UPROPERTY(EditAnywhere, Category = Flex)
	bool AttachToRigids;

	/** The per-particle mass to use for the particles, for clothing this value be multiplied by 0-1 dependent on the vertex color */
	UPROPERTY(EditAnywhere, Category = Flex)
	float Mass;
	
	// particles created from the mesh
	UPROPERTY()
	TArray<FVector4> Particles;

	// distance constraints
	UPROPERTY()
	TArray<int32> SpringIndices;
	UPROPERTY()
	TArray<float> SpringCoefficients;
	UPROPERTY()
	TArray<float> SpringRestLengths;

	// faces for cloth
	UPROPERTY()
	TArray<int32> Triangles;
	UPROPERTY()
	TArray<int32> VertexToParticleMap;

	// shape constraints
	UPROPERTY()
	TArray<int32> ShapeIndices;
	UPROPERTY()
	TArray<int32> ShapeOffsets;
	UPROPERTY()
	TArray<float> ShapeCoefficients;
	UPROPERTY()
	TArray<FVector> ShapeCenters;

	// mesh skinning information
	UPROPERTY()
	TArray<float> SkinningWeights;
	UPROPERTY()
	TArray<int32> SkinningIndices;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

	virtual void ReImport(const UStaticMesh* Parent) {}
	virtual const NvFlexExtAsset* GetFlexAsset() { return NULL; }

	//
	NvFlexExtAsset* Asset;

};
