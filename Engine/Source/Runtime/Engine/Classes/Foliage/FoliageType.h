// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Curves/CurveFloat.h"
#include "FoliageType.generated.h"

UENUM()
enum FoliageVertexColorMask
{
	FOLIAGEVERTEXCOLORMASK_Disabled,
	FOLIAGEVERTEXCOLORMASK_Red,
	FOLIAGEVERTEXCOLORMASK_Green,
	FOLIAGEVERTEXCOLORMASK_Blue,
	FOLIAGEVERTEXCOLORMASK_Alpha,
};


UCLASS(hidecategories = Object, editinlinenew, MinimalAPI, BlueprintType, Blueprintable)
class UFoliageType : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Painting)
	float Density;

	UPROPERTY(EditAnywhere, Category=Painting)
	float Radius;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ScaleMinX;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ScaleMinY;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ScaleMinZ;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ScaleMaxX;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ScaleMaxY;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ScaleMaxZ;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 LockScaleX:1;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 LockScaleY:1;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 LockScaleZ:1;

	UPROPERTY(EditAnywhere, Category=General, meta=(Subcategory = "Placement"))
	float AlignMaxAngle;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	float RandomPitchAngle;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	float GroundSlope;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	float HeightMin;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	float HeightMax;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	FName LandscapeLayer;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	uint32 AlignToNormal:1;

	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Placement"))
	uint32 RandomYaw:1;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 UniformScale:1;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ZOffsetMin;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ZOffsetMax;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 CollisionWithWorld:1;

	UPROPERTY(EditAnywhere, Category=Painting)
	FVector CollisionScale;

	UPROPERTY()
	FBoxSphereBounds MeshBounds;

	// X, Y is origin position and Z is radius...
	UPROPERTY()
	FVector LowBoundOriginRadius;

	UPROPERTY(EditAnywhere, Category=Painting)
	TEnumAsByte<enum FoliageVertexColorMask> VertexColorMask;

	UPROPERTY(EditAnywhere, Category=Painting)
	float VertexColorMaskThreshold;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 VertexColorMaskInvert:1;

	UPROPERTY()
	float ReapplyDensityAmount;

	UPROPERTY()
	uint32 ReapplyDensity:1;

	UPROPERTY()
	uint32 ReapplyRadius:1;

	UPROPERTY()
	uint32 ReapplyAlignToNormal:1;

	UPROPERTY()
	uint32 ReapplyRandomYaw:1;

	UPROPERTY()
	uint32 ReapplyScaleX:1;

	UPROPERTY()
	uint32 ReapplyScaleY:1;

	UPROPERTY()
	uint32 ReapplyScaleZ:1;

	UPROPERTY()
	uint32 ReapplyRandomPitchAngle:1;

	UPROPERTY()
	uint32 ReapplyGroundSlope:1;

	UPROPERTY()
	uint32 ReapplyHeight:1;

	UPROPERTY()
	uint32 ReapplyLandscapeLayer:1;

	UPROPERTY()
	uint32 ReapplyZOffset:1;

	UPROPERTY()
	uint32 ReapplyCollisionWithWorld:1;

	UPROPERTY()
	uint32 ReapplyVertexColorMask:1;

	/* The distance where instances will begin to fade out if using a PerInstanceFadeAmount material node. 0 disables. */
	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Culling"))
	int32 StartCullDistance;

	/**
	 * The distance where instances will have completely faded out when using a PerInstanceFadeAmount material node. 0 disables. 
	 * When the entire cluster is beyond this distance, the cluster is completely culled and not rendered at all.
	 */
	UPROPERTY(EditAnywhere, Category = General, meta = (Subcategory = "Culling"))
	int32 EndCullDistance;

	UPROPERTY()
	int32 DisplayOrder;

	UPROPERTY()
	uint32 IsSelected:1;

	UPROPERTY()
	uint32 ShowNothing:1;

	UPROPERTY()
	uint32 ShowPaintSettings:1;

	UPROPERTY()
	uint32 ShowInstanceSettings:1;

	/** Controls whether the foliage should cast a shadow or not. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting"))
	uint32 CastShadow:1;

	/** Controls whether the foliage should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting"))
	uint32 bAffectDynamicIndirectLighting:1;

	/** Controls whether the primitive should affect dynamic distance field lighting methods.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting", EditCondition = "CastShadow"))
	uint32 bAffectDistanceFieldLighting:1;

	/** Controls whether the foliage should cast shadows in the case of non precomputed shadowing.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting"))
	uint32 bCastDynamicShadow:1;

	/** Whether the foliage should cast a static shadow from shadow casting lights.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting"))
	uint32 bCastStaticShadow:1;

	/** Whether this foliage should cast dynamic shadows as if it were a two sided material. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting"))
	uint32 bCastShadowAsTwoSided:1;

	/** Whether the foliage receives decals. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = General, meta = (Subcategory = "Rendering"))
	uint32 bReceivesDecals : 1;
	
	/** Whether to override the lightmap resolution defined in the static mesh. */
	UPROPERTY(BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting"))
	uint32 bOverrideLightMapRes:1;

	/** Overrides the lightmap resolution defined in the static mesh. A value of 0 disables static lighting/shadowing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (Subcategory = "Lighting", DisplayName = "Light Map Resolution", EditCondition = "bOverrideLightMapRes"))
	int32 OverriddenLightMapRes;

	/** Custom collision for foliage */
	UPROPERTY(EditAnywhere, Category = Painting, meta = (Subcategory = "Collision", HideObjectType = true))
	struct FBodyInstance BodyInstance;

	/* Gets the mesh associated with this FoliageType */
	virtual UStaticMesh* GetStaticMesh() PURE_VIRTUAL(UFoliageType::GetStaticMesh, return nullptr; );

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Ecosystem specific parameters */

	/** Specifies the number of seeds to populate along 10 meters. The number is implicitly squared to cover a 10m x 10m area*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float InitialSeedDensity;

	/** The average distance between the spreading instance and its seeds. For example, a tree with an AverageSpreadDistance 10 will ensure the average distance between the tree and its seeds is 10cm */
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float AverageSpreadDistance;

	/** Specifies how much seed distance varies from the average. For example, a tree with an AverageSpreadDistance 10 and a SpreadVariance 1 will produce seeds with an average distance of 10cm plus or minus 1cm */
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float SpreadVariance;

	/** The CollisionRadius determines when two instances overlap. When two instances overlap a winner will be picked based on rules and priority. */
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Collision", ClampMin = "0.0", UIMin = "0.0"))
	float CollisionRadius;

	/** The ShadeRadius determines when two instances overlap. If an instance can grow in the shade this radius is ignored.*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Collision", ClampMin = "0.0", UIMin = "0.0"))
	float ShadeRadius;

	/** The number of times we age the species and spread its seeds. */
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0", UIMin = "0"))
	int32 NumSteps;

	/** The number of seeds an instance will spread in a single step of the simulation. */
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0", UIMin = "0"))
	int32 SeedsPerStep;

	/** Whether the species can grow in shade. If this is true shade radius is ignored during overlap tests*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Growth"))
	bool bGrowsInShade;

	/** The minimum scale that an instance will be. Corresponds to age = 0*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float MinScale;

	/** The maximum scale that a seed will grow to. Corresponds to age = 1*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float MaxScale;

	/** Specifies the oldest a new seed can be. The new seed will have an age randomly distributed in [0,InitMaxAge] */
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float InitialMaxAge;

	/** Specifies the oldest a seed can be. After reaching this age the instance will still spread seeds, but will not get any older*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float MaxAge;

	/** When two instances overlap we must determine which instance to remove. The instance with a lower OverlapPriority will be removed. In the case where OverlapPriority is the same regular simulation rules apply.*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Growth"))
	float OverlapPriority;

	/** The set of meshes to be used for representing an instance placed in the world.*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Rendering"))
	TArray<UStaticMesh*> StaticMeshes;


	UPROPERTY()
	int32 ChangeCount;

	
	ENGINE_API float GetSeedDensitySquared() const { return InitialSeedDensity * InitialSeedDensity; }
	ENGINE_API float GetMaxRadius() const;
	ENGINE_API float GetScaleForAge(const float Age) const;
	ENGINE_API float GetInitAge(FRandomStream& RandomStream) const;
	ENGINE_API float GetNextAge(const float CurrentAge, const int32 NumSteps) const;

private:

	/** The curve used to interpolate the instance scale.*/
	UPROPERTY(Category = Ecosystem, EditAnywhere, meta = (Subcategory = "Rendering"))
	FRuntimeFloatCurve ScaleCurve;
};


