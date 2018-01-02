// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.generated.h"


UENUM()
enum class ENiagaraSortMode : uint8
{
	/** Perform no additional sorting prior to rendering.*/
	SortNone,
	/** Sort by depth to the camera's near plane.*/
	SortViewDepth,
	/** Sort by distance to the camera's origin.*/
	SortViewDistance
};

/** This enum decides how a sprite particle will orient its "up" axis. Must keep these in sync with NiagaraSpriteVertexFactory.ush*/
UENUM()
enum class ENiagaraSpriteAlignment : uint8
{
	/** Only Particles.SpriteRotation and FacingMode impact the alignment of the particle.*/
	Unaligned,
	/** Imagine the particle texture having an arrow pointing up, this mode makes the arrow point in the direction of the Particles.Velocity attribute. FacingMode is ignored unless CustomFacingVector is set.*/
	VelocityAligned,
	/** Imagine the particle texture having an arrow pointing up, this mode makes the arrow point towards the axis defined by the "Particles.SpriteAlignment" attribute. FacingMode is ignored unless CustomFacingVector is set. If the "Particles.SpriteAlignment" attribute is missing, this falls back to Unaligned mode.*/
	CustomAlignment
};


/** This enum decides how a sprite particle will orient its "facing" axis. Must keep these in sync with NiagaraSpriteVertexFactory.ush*/
UENUM()
enum class ENiagaraSpriteFacingMode : uint8
{
	/** The sprite billboard origin is always "looking at" the camera origin, trying to keep its up axis aligned to the camera's up axis. */
	FaceCamera,
	/** The sprite billboard plane is completely parallel to the camera plane. Particle always looks "flat" */
	FaceCameraPlane,
	/** The sprite billboard faces toward the "Particles.SpriteFacing" vector attribute, using the per-axis CustomFacingVectorMask as a lerp factor from the standard FaceCamera mode. If the "Particles.SpriteFacing" attribute is missing, this falls back to FaceCamera mode.*/
	CustomFacingVector,
	/** Faces the camera position, but is not dependent on the camera rotation.  This method produces more stable particles under camera rotation. Uses the up axis of (0,0,1).*/
	FaceCameraPosition,
	/** Blends between FaceCamera and FaceCameraPosition.*/
	FaceCameraDistanceBlend
};


UCLASS(editinlinenew)
class UNiagaraSpriteRendererProperties : public UNiagaraRendererProperties
{
public:
	GENERATED_BODY()

	UNiagaraSpriteRendererProperties();

	//~ UNiagaraRendererProperties interface
	virtual NiagaraRenderer* CreateEmitterRenderer(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const override;
#if WITH_EDITORONLY_DATA
	virtual bool IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage) override;
	virtual void FixMaterial(UMaterial* Material) override;
	virtual const TArray<FNiagaraVariable>& GetRequiredAttributes() override;
	virtual const TArray<FNiagaraVariable>& GetOptionalAttributes() override;
#endif // WITH_EDITORONLY_DATA

	/** The material used to render the particle. Note that it must have the Use with Niagara Sprites flag checked.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	UMaterialInterface* Material;
	
	/** Imagine the particle texture having an arrow pointing up, these modes define how the particle aligns that texture to other particle attributes.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	ENiagaraSpriteAlignment Alignment;

	/** Determines how the particle billboard orients itself relative to the camera.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	ENiagaraSpriteFacingMode FacingMode;

	/** Used as a per-axis interpolation factor with the CustomFacingVector mode to determine how the billboard orients itself relative to the camera. A value of 1.0 is fully facing the custom vector. A value of 0.0 uses the standard facing strategy.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector CustomFacingVectorMask;

	/** Determines the location of the pivot point of this particle. It follows Unreal's UV space, which has the upper left of the image at 0,0 and bottom right at 1,1. The middle is at 0.5, 0.5. */
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector2D PivotInUVSpace;

	/** Determines how we sort the particles prior to rendering.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	ENiagaraSortMode SortMode;
	
	/** When using SubImage lookups for particles, this variable contains the number of columns in X and the number of rows in Y.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector2D SubImageSize;

	/** If true, blends the sub-image UV lookup with its next adjacent member using the fractional part of the SubImageIndex float value as the linear interpolation factor.*/
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering", meta = (DisplayName = "Sub UV Blending Enabled"))
	uint32 bSubImageBlend : 1;

	/** If true, removes the HMD view roll (e.g. in VR) */
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering", meta = (DisplayName = "Remove HMD Roll"))
	uint32 bRemoveHMDRollInVR : 1;

	/** The distance at which FacingCameraDistanceBlend	is fully FacingCamera */
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering", meta = (UIMin = "0"))
	float MinFacingCameraBlendDistance;

	/** The distance at which FacingCameraDistanceBlend	is fully FacingCameraPosition */
	UPROPERTY(EditAnywhere, Category = "Sprite Rendering", meta = (UIMin = "0"))
	float MaxFacingCameraBlendDistance;
};



