// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SplineMeshComponent.generated.h"


UENUM()
namespace ESplineMeshAxis
{
	enum Type
	{
		X,
		Y,
		Z,
	};
}

/** 
 * Structure that holds info about spline, passed to renderer to deform UStaticMesh.
 * Also used by Lightmass, so be sure to update Lightmass::FSplineMeshParams and the static lighting code if this changes!
 */
USTRUCT()
struct FSplineMeshParams
{
	GENERATED_USTRUCT_BODY()

	/** Start location of spline, in component space */
	UPROPERTY()
	FVector StartPos;

	/** Start tangent of spline, in component space */
	UPROPERTY()
	FVector StartTangent;

	/** X and Y scale applied to mesh at start of spline */
	UPROPERTY()
	FVector2D StartScale;

	/** Roll around spline applied at start */
	UPROPERTY()
	float StartRoll;

	/** Starting offset of the mesh from the spline, in component space */
	UPROPERTY()
	FVector2D StartOffset;

	/** End location of spline, in component space */
	UPROPERTY()
	FVector EndPos;

	/** End tangent of spline, in component space */
	UPROPERTY()
	FVector EndTangent;

	/** X and Y scale applied to mesh at end of spline */
	UPROPERTY()
	FVector2D EndScale;

	/** Roll around spline applied at end */
	UPROPERTY()
	float EndRoll;

	/** Ending offset of the mesh from the spline, in component space */
	UPROPERTY()
	FVector2D EndOffset;


	FSplineMeshParams()
		: StartPos(ForceInit)
		, StartTangent(ForceInit)
		, StartScale(ForceInit)
		, StartRoll(0)
		, StartOffset(ForceInit)
		, EndPos(ForceInit)
		, EndTangent(ForceInit)
		, EndScale(ForceInit)
		, EndRoll(0)
		, EndOffset(ForceInit)
	{
	}

};

UCLASS(HeaderGroup=Component, MinimalAPI)
class USplineMeshComponent : public UStaticMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

	/** Spline that is used to deform mesh */
	UPROPERTY()
	struct FSplineMeshParams SplineParams;

	/** Axis (in component space) that is used to determine X axis for co-ordinates along spline */
	UPROPERTY()
	FVector SplineXDir;

	/** If true, will use smooth interpolation (ease in/out) for Scale, Roll, and Offset along this section of spline. If false, uses linear */
	UPROPERTY()
	uint32 bSmoothInterpRollScale:1;

	/** Chooses the forward axis for the spline mesh orientation */
	UPROPERTY()
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis;

	// Physics data.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, editinline, Category=Physics)
	class UBodySetup* BodySetup;

#if WITH_EDITORONLY_DATA
	// Used to automatically trigger rebuild of collision data
	UPROPERTY()
	FGuid CachedMeshBodySetupGuid;

	UPROPERTY(transient)
	uint32 bSelected:1;
#endif

	//Begin UObject Interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//Begin USceneComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	//End USceneComponent Interface

	//Begin UPrimitiveComponent Interface
	virtual void CreatePhysicsState() OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
#if WITH_EDITOR
	virtual bool ShouldRenderSelected() const OVERRIDE
	{
		return Super::ShouldRenderSelected() || bSelected;
	}
#endif
	virtual bool DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const OVERRIDE;
	//End UPrimitiveComponent Interface

	//Begin UStaticMeshComponent Interface
	virtual class FStaticMeshStaticLightingMesh* AllocateStaticLightingMesh(int32 LODIndex, const TArray<ULightComponent*>& InRelevantLights);
	//End UStaticMeshComponent Interface

	// Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) OVERRIDE;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const OVERRIDE;
	virtual bool WantsNegXTriMesh() OVERRIDE { return false; }
	// End Interface_CollisionDataProvider Interface

#if WITH_EDITOR
	// Builds collision for the spline mesh (if collision is enabled)
	void RecreateCollision();
#endif

	/**
	 * Calculates the spline transform, including roll, scale, and offset along the spline at a specified distance
	 * @Note:  This is mirrored to Lightmass::CalcSliceTransform() and LocalVertexShader.usf.  If you update one of these, please update them all! 
	 */
	FTransform CalcSliceTransform(const float DistanceAlong) const;
};



