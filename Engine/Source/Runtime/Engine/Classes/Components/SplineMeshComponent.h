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
	UPROPERTY(EditAnywhere, Category=SplineMesh)
	FVector StartPos;

	/** Start tangent of spline, in component space */
	UPROPERTY(EditAnywhere, Category=SplineMesh)
	FVector StartTangent;

	/** X and Y scale applied to mesh at start of spline */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
	FVector2D StartScale;

	/** Roll around spline applied at start */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
	float StartRoll;

	/** Starting offset of the mesh from the spline, in component space */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
	FVector2D StartOffset;

	/** End location of spline, in component space */
	UPROPERTY(EditAnywhere, Category=SplineMesh)
	FVector EndPos;

	/** End tangent of spline, in component space */
	UPROPERTY(EditAnywhere, Category=SplineMesh)
	FVector EndTangent;

	/** X and Y scale applied to mesh at end of spline */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
	FVector2D EndScale;

	/** Roll around spline applied at end */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
	float EndRoll;

	/** Ending offset of the mesh from the spline, in component space */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
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

UCLASS(ClassGroup=Rendering, hidecategories=(Physics), meta=(BlueprintSpawnableComponent))
class ENGINE_API USplineMeshComponent : public UStaticMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

	/** Spline that is used to deform mesh */
	UPROPERTY(EditAnywhere, Category=SplineMesh, meta=(ShowOnlyInnerProperties))
	FSplineMeshParams SplineParams;

	/** Axis (in component space) that is used to determine X axis for co-ordinates along spline */
	UPROPERTY(EditAnywhere, Category=SplineMesh)
	FVector SplineUpDir;

	/** If true, will use smooth interpolation (ease in/out) for Scale, Roll, and Offset along this section of spline. If false, uses linear */
	UPROPERTY(EditAnywhere, Category=SplineMesh, AdvancedDisplay)
	uint32 bSmoothInterpRollScale:1;

	/** Chooses the forward axis for the spline mesh orientation */
	UPROPERTY(EditAnywhere, Category=SplineMesh)
	TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis;

	// Physics data.
	UPROPERTY()
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
	virtual bool Modify(bool bAlwaysMarkDirty = true) OVERRIDE;
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

	/** Called when spline params are changed, to notify render thread and possibly collision */
	void MarkSplineParamsDirty();

	/** Set the start position of spline in local space */
	UFUNCTION(BlueprintCallable, Category=SplineMesh)
	void SetStartPosition(FVector StartPos);

	/** Set the start tangent vector of spline in local space */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetStartTangent(FVector StartTangent);

	/** Set the end position of spline in local space */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetEndPosition(FVector EndPos);

	/** Set the end tangent vector of spline in local space */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetEndTangent(FVector EndTangent);

	/** Set the start and end, position and tangent, all in local space */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetStartAndEnd(FVector StartPos, FVector StartTangent, FVector EndPos, FVector EndTangent);

	/** Set the start scaling */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetStartScale(FVector2D StartScale = FVector2D(1,1));

	/** Set the start roll */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetStartRoll(float StartRoll);

	/** Set the start offset */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetStartOffset(FVector2D StartOffset);

	/** Set the end scaling */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetEndScale(FVector2D EndScale = FVector2D(1,1));

	/** Set the end roll */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetEndRoll(float EndRoll);

	/** Set the end offset */
	UFUNCTION(BlueprintCallable, Category = SplineMesh)
	void SetEndOffset(FVector2D EndOffset);



	// Destroys the body setup, used to clear collision if the mesh goes missing
	void DestroyBodySetup();
#if WITH_EDITOR
	// Builds collision for the spline mesh (if collision is enabled)
	void RecreateCollision();
#endif

	/**
	 * Calculates the spline transform, including roll, scale, and offset along the spline at a specified distance
	 * @Note:  This is mirrored to Lightmass::CalcSliceTransform() and LocalVertexShader.usf.  If you update one of these, please update them all! 
	 */
	FTransform CalcSliceTransform(const float DistanceAlong) const;

	static const float& GetAxisValue(const FVector& InVector, ESplineMeshAxis::Type InAxis);
	static float& GetAxisValue(FVector& InVector, ESplineMeshAxis::Type InAxis);

};



