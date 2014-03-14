// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InstancedFoliageSettings.generated.h"

UENUM()
enum FoliageVertexColorMask
{
	FOLIAGEVERTEXCOLORMASK_Disabled,
	FOLIAGEVERTEXCOLORMASK_Red,
	FOLIAGEVERTEXCOLORMASK_Green,
	FOLIAGEVERTEXCOLORMASK_Blue,
	FOLIAGEVERTEXCOLORMASK_Alpha,
};


UCLASS(HeaderGroup=Foliage, hidecategories=Object, editinlinenew, MinimalAPI)
class UInstancedFoliageSettings : public UObject
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

	UPROPERTY(EditAnywhere, Category=Painting)
	float AlignMaxAngle;

	UPROPERTY(EditAnywhere, Category=Painting)
	float RandomPitchAngle;

	UPROPERTY(EditAnywhere, Category=Painting)
	float GroundSlope;

	UPROPERTY(EditAnywhere, Category=Painting)
	float HeightMin;

	UPROPERTY(EditAnywhere, Category=Painting)
	float HeightMax;

	UPROPERTY(EditAnywhere, Category=Painting)
	FName LandscapeLayer;

	UPROPERTY(EditAnywhere, Category=Painting)
	uint32 AlignToNormal:1;

	UPROPERTY(EditAnywhere, Category=Painting)
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

	UPROPERTY(EditAnywhere, Category=Clustering)
	int32 MaxInstancesPerCluster;

	UPROPERTY(EditAnywhere, Category=Clustering)
	float MaxClusterRadius;

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

	UPROPERTY(EditAnywhere, Category=Culling)
	int32 StartCullDistance;

	UPROPERTY(EditAnywhere, Category=Culling)
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

	/** Controls whether the primitive component should cast a shadow or not. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting)
	uint32 CastShadow:1;

	/** Controls whether the primitive should inject light into the Light Propagation Volume.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting)
	uint32 bAffectDynamicIndirectLighting:1;

	/** Controls whether the primitive should cast shadows in the case of non precomputed shadowing.  This flag is only used if CastShadow is true. **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting)
	uint32 bCastDynamicShadow:1;

	/** Whether the object should cast a static shadow from shadow casting lights.  This flag is only used if CastShadow is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Lighting)
	uint32 bCastStaticShadow:1;

	/** 
	 *	If true, the primitive will cast shadows even if bHidden is true.
	 *	Controls whether the primitive should cast shadows when hidden.
	 *	This flag is only used if CastShadow is true.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bCastHiddenShadow:1;

	/** Whether this primitive should cast dynamic shadows as if it were a two sided material. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category=Lighting)
	uint32 bCastShadowAsTwoSided:1;
};


