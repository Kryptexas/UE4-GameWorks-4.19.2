// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Curves/CurveFloat.h"
#include "FoliageType.generated.h"

UENUM()
enum FoliageVertexColorMask
{
	FOLIAGEVERTEXCOLORMASK_Disabled UMETA(DisplayName="Disabled"),
	FOLIAGEVERTEXCOLORMASK_Red		UMETA(DisplayName="Red"),
	FOLIAGEVERTEXCOLORMASK_Green	UMETA(DisplayName="Green"),
	FOLIAGEVERTEXCOLORMASK_Blue		UMETA(DisplayName="Blue"),
	FOLIAGEVERTEXCOLORMASK_Alpha	UMETA(DisplayName="Alpha"),
};

namespace EHasCustomNavigableGeometry
{
	enum Type;
}
UENUM()
enum class EFoliageScaling : uint8
{
	// Foliage instances will have uniform X,Y and Z scales
	Uniform,
	// Foliage instances will have random X,Y and Z scales
	Free,
	// Locks the X and Y axis scale
	LockXY,
	// Locks the X and Z axis scale
	LockXZ,
	// Locks the Y and Z axis scale
	LockYZ
};


UCLASS(hidecategories = Object, editinlinenew, MinimalAPI, BlueprintType, Blueprintable)
class UFoliageType : public UObject
{
	GENERATED_UCLASS_BODY()

	/* A GUID that is updated every time the foliage type is modified, 
	   so foliage placed in the level can detect the FoliageType has changed. */
	UPROPERTY()
	FGuid UpdateGuid;

	/** Foliage instances will be placed at this density, specified in instances per 1000x1000 unit area */
	UPROPERTY(EditAnywhere, Category=Painting, meta=(DisplayName="Density / 1Kuu", UIMin=0, ClampMin=0))
	float Density;

	/** The minimum distance between foliage instances */
	UPROPERTY(EditAnywhere, Category=Painting, meta=(UIMin=0, ClampMin=0))
	float Radius;

	/** Specified foliage instance scaling behavior */
	UPROPERTY(EditAnywhere, Category=Painting)
	EFoliageScaling Scaling;

	/** Specifies the range of scale, from minimum to maximum, to apply to a foliage instance's X Scale property */
	UPROPERTY(EditAnywhere, Category=Painting, meta=(UIMin=0.001))
	FFloatInterval ScaleX;

	/** Specifies the range of scale, from minimum to maximum, to apply to a foliage instance's Y Scale property */
	UPROPERTY(EditAnywhere, Category=Painting, meta=(UIMin=0.001))
	FFloatInterval ScaleY;

	/** Specifies the range of scale, from minimum to maximum, to apply to a foliage instance's Z Scale property */
	UPROPERTY(EditAnywhere, Category=Painting, meta=(UIMin=0.001))
	FFloatInterval ScaleZ;

	/** Whether foliage instances should have their angle adjusted away from vertical to match the normal of the surface they're painted on */
	UPROPERTY(EditAnywhere, Category=Placement)
	uint32 AlignToNormal:1;

	/** The maximum angle in degrees that foliage instances will be adjusted away from the vertical */
	UPROPERTY(EditAnywhere, Category=Placement)
	float AlignMaxAngle;

	/** A random pitch adjustment can be applied to each instance, up to the specified angle in degrees, from the original vertical */
	UPROPERTY(EditAnywhere, Category=Placement)
	float RandomPitchAngle;

	/* Foliage instances will only be placed on surfaces sloping in the specified angle range from the horizontal */
	UPROPERTY(EditAnywhere, Category=Placement, meta=(UIMin=0))
	FFloatInterval GroundSlopeAngle;

	/* The valid altitude range where foliage instances will be placed, specified using minimum and maximum world coordinate Z values */
	UPROPERTY(EditAnywhere, Category=Placement)
	FFloatInterval Height;

	/** If a layer name is specified, painting on landscape will limit the foliage to areas of landscape with the specified layer painted */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Placement)
	TArray<FName> LandscapeLayers;

	UPROPERTY()
	FName LandscapeLayer_DEPRECATED;
	
	/** Specifies the minimum value above which the landscape layer weight value must be, in order for foliage instances to be placed in a specific area */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Placement)
	float MinimumLayerWeight;

	/** If selected, foliage instances will have a random yaw rotation around their vertical axis applied */
	UPROPERTY(EditAnywhere, Category=Placement)
	uint32 RandomYaw:1;

	/** Specifies a range from minimum to maximum of the offset to apply to a foliage instance's Z location */
	UPROPERTY(EditAnywhere, Category=Painting, meta=(DisplayName="Z Offset"))
	FFloatInterval ZOffset;
	
	/* If checked, an overlap test with existing world geometry is performed before each instance is placed */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Painting)
	uint32 CollisionWithWorld:1;

	/* The foliage instance's collision bounding box will be scaled by the specified amount before performing the overlap check */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Painting)
	FVector CollisionScale;

	UPROPERTY()
	FBoxSphereBounds MeshBounds;

	// X, Y is origin position and Z is radius...
	UPROPERTY()
	FVector LowBoundOriginRadius;

	/** 
	 *  When painting on static meshes, foliage instance placement can be limited to areas where the static mesh has values in the selected vertex color channel(s). 
	 *  This allows a static mesh to mask out certain areas to prevent foliage from being placed there
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Painting)
	TEnumAsByte<enum FoliageVertexColorMask> VertexColorMask;

	/** Specifies the threshold value above which the static mesh vertex color value must be, in order for foliage instances to be placed in a specific area */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Painting)
	float VertexColorMaskThreshold;

	/** 
	 *  When unchecked, foliage instances will be placed only when the vertex color in the specified channel(s) is above the threshold amount. 
	 *  When checked, the vertex color must be less than the threshold amount 
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Painting)
	uint32 VertexColorMaskInvert:1;

	UPROPERTY(EditAnywhere, Category=Painting)
	float ReapplyDensityAmount;

	/** If checked, the density of foliage instances already placed will be adjusted. <1 will remove instances to reduce the density, >1 will place extra instances */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyDensity:1;

	/** If checked, foliage instances not meeting the new Radius constraint will be removed */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyRadius:1;

	/** If checked, foliage instances will have their normal alignment adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyAlignToNormal:1;

	/** If checked, foliage instances will have their yaw adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyRandomYaw:1;

	/** If checked, foliage instances will have their X scale adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyScaleX:1;

	/** If checked, foliage instances will have their Y scale adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyScaleY:1;

	/** If checked, foliage instances will have their Z scale adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyScaleZ:1;

	/** If checked, foliage instances will have their pitch adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyRandomPitchAngle:1;

	/** If checked, foliage instances not meeting the ground slope condition will be removed by the Reapply too */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyGroundSlope:1;

	/* If checked, foliage instances not meeting the valid Z height condition will be removed by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyHeight:1;

	/* If checked, foliage instances painted on areas that do not have the appopriate landscape layer painted will be removed by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyLandscapeLayer:1;

	/** If checked, foliage instances will have their Z offset adjusted by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyZOffset:1;

	/* If checked, foliage instances will have an overlap test with the world reapplied, and overlapping instances will be removed by the Reapply tool */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyCollisionWithWorld:1;

	/* If checked, foliage instances no longer matching the vertex color constraint will be removed by the Reapply too */
	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 ReapplyVertexColorMask:1;

	/**
	 * The distance where instances will begin to fade out if using a PerInstanceFadeAmount material node. 0 disables.
	 * When the entire cluster is beyond this distance, the cluster is completely culled and not rendered at all.
	 */
	UPROPERTY(EditAnywhere, Category=InstanceSettings, meta=(UIMin=0))
	FInt32Interval CullDistance;

	/** Controls whether the foliage should take part in static lighting/shadowing. If false, mesh will not receive or cast static lighting or shadows, regardless of other settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	uint32 bEnableStaticLighting : 1;

	/** Controls whether the foliage should cast a shadow or not. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	uint32 CastShadow:1;

	/** Controls whether the foliage should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	uint32 bAffectDynamicIndirectLighting:1;

	/** Controls whether the primitive should affect dynamic distance field lighting methods.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta = (EditCondition = "CastShadow"))
	uint32 bAffectDistanceFieldLighting:1;

	/** Controls whether the foliage should cast shadows in the case of non precomputed shadowing.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	uint32 bCastDynamicShadow:1;

	/** Whether the foliage should cast a static shadow from shadow casting lights.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings)
	uint32 bCastStaticShadow:1;

	/** Whether this foliage should cast dynamic shadows as if it were a two sided material. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings)
	uint32 bCastShadowAsTwoSided:1;

	/** Whether the foliage receives decals. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=InstanceSettings)
	uint32 bReceivesDecals : 1;
	
	/** Whether to override the lightmap resolution defined in the static mesh. */
	UPROPERTY(BlueprintReadOnly, Category=InstanceSettings)
	uint32 bOverrideLightMapRes:1;

	/** Overrides the lightmap resolution defined in the static mesh */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=InstanceSettings, meta = (DisplayName = "Light Map Resolution", EditCondition = "bOverrideLightMapRes"))
	int32 OverriddenLightMapRes;

	/** Custom collision for foliage */
	UPROPERTY(EditAnywhere, Category=InstanceSettings, meta = (HideObjectType = true))
	struct FBodyInstance BodyInstance;

	/** Force navmesh */
	UPROPERTY(EditAnywhere, Category=InstanceSettings, meta = (HideObjectType = true))
	TEnumAsByte<EHasCustomNavigableGeometry::Type> CustomNavigableGeometry;

	UPROPERTY()
	int32 DisplayOrder;

	UPROPERTY()
	uint32 IsSelected:1;

	/* Gets/Sets the mesh associated with this FoliageType */
	virtual UStaticMesh* GetStaticMesh() const PURE_VIRTUAL(UFoliageType::GetStaticMesh, return nullptr; );
	virtual void SetStaticMesh(UStaticMesh* InStaticMesh) PURE_VIRTUAL(UFoliageType::SetStaticMesh,);

#if WITH_EDITOR
	/* Lets subclasses decide if the InstancedFoliageActor should reallocate its instances if the specified property change event occurs */
	virtual bool IsFoliageReallocationRequiredForPropertyChange(struct FPropertyChangedEvent& PropertyChangedEvent) const { return true; }

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Procedural specific parameters */

	/** Specifies the number of seeds to populate along 10 meters. The number is implicitly squared to cover a 10m x 10m area*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float InitialSeedDensity;

	/** The average distance between the spreading instance and its seeds. For example, a tree with an AverageSpreadDistance 10 will ensure the average distance between the tree and its seeds is 10cm */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float AverageSpreadDistance;

	/** Specifies how much seed distance varies from the average. For example, a tree with an AverageSpreadDistance 10 and a SpreadVariance 1 will produce seeds with an average distance of 10cm plus or minus 1cm */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0.0", UIMin = "0.0"))
	float SpreadVariance;

	/** The CollisionRadius determines when two instances overlap. When two instances overlap a winner will be picked based on rules and priority. */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Collision", ClampMin = "0.0", UIMin = "0.0"))
	float CollisionRadius;

	/** The ShadeRadius determines when two instances overlap. If an instance can grow in the shade this radius is ignored.*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Collision", ClampMin = "0.0", UIMin = "0.0"))
	float ShadeRadius;

	/** The number of times we age the species and spread its seeds. */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0", UIMin = "0"))
	int32 NumSteps;

	/** The number of seeds an instance will spread in a single step of the simulation. */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering", ClampMin = "0", UIMin = "0"))
	int32 SeedsPerStep;

	/** The seed that determines placement of initial seeds. */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering"))
	int32 DistributionSeed;

	/** The seed that determines placement of initial seeds. */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Clustering"))
	float MaxInitialSeedOffset;

	/** Whether the species can grow in shade. If this is true shade radius is ignored during overlap tests*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth"))
	bool bGrowsInShade;

	/** The minimum scale that an instance will be. Corresponds to age = 0*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float MinScale;

	/** The maximum scale that a seed will grow to. Corresponds to age = 1*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float MaxScale;

	/** Specifies the oldest a new seed can be. The new seed will have an age randomly distributed in [0,InitMaxAge] */
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float InitialMaxAge;

	/** Specifies the oldest a seed can be. After reaching this age the instance will still spread seeds, but will not get any older*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth", ClampMin = "0.0", UIMin = "0.0"))
	float MaxAge;

	/** When two instances overlap we must determine which instance to remove. The instance with a lower OverlapPriority will be removed. In the case where OverlapPriority is the same regular simulation rules apply.*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth"))
	float OverlapPriority;

	UPROPERTY()
	int32 ChangeCount;

	
	FOLIAGE_API float GetSeedDensitySquared() const { return InitialSeedDensity * InitialSeedDensity; }
	FOLIAGE_API float GetMaxRadius() const;
	FOLIAGE_API float GetScaleForAge(const float Age) const;
	FOLIAGE_API float GetInitAge(FRandomStream& RandomStream) const;
	FOLIAGE_API float GetNextAge(const float CurrentAge, const int32 NumSteps) const;

	virtual void Serialize(FArchive& Ar) override;

	FOLIAGE_API FVector GetRandomScale() const;

private:

	/** The curve used to interpolate the instance scale.*/
	UPROPERTY(Category = Procedural, EditAnywhere, meta = (Subcategory = "Growth"))
	FRuntimeFloatCurve ScaleCurve;
	
	// Deprecated since FFoliageCustomVersion::FoliageTypeCustomization
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	float ScaleMinX_DEPRECATED;

	UPROPERTY()
	float ScaleMinY_DEPRECATED;

	UPROPERTY()
	float ScaleMinZ_DEPRECATED;

	UPROPERTY()
	float ScaleMaxX_DEPRECATED;

	UPROPERTY()
	float ScaleMaxY_DEPRECATED;

	UPROPERTY()
	float ScaleMaxZ_DEPRECATED;

	UPROPERTY()
	float HeightMin_DEPRECATED;

	UPROPERTY()
	float HeightMax_DEPRECATED;

	UPROPERTY()
	float ZOffsetMin_DEPRECATED;

	UPROPERTY()
	float ZOffsetMax_DEPRECATED;

	UPROPERTY()
	int32 StartCullDistance_DEPRECATED;
	
	UPROPERTY()
	int32 EndCullDistance_DEPRECATED;

	UPROPERTY()
	uint32 UniformScale_DEPRECATED:1;

	UPROPERTY()
	uint32 LockScaleX_DEPRECATED:1;

	UPROPERTY()
	uint32 LockScaleY_DEPRECATED:1;

	UPROPERTY()
	uint32 LockScaleZ_DEPRECATED:1;

	UPROPERTY()
	float GroundSlope_DEPRECATED;
	
	UPROPERTY()
	float MinGroundSlope_DEPRECATED;
#endif// WITH_EDITORONLY_DATA
};


