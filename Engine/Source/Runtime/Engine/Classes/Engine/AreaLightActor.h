// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/StaticMeshActor.h"
#include "AreaLightActor.generated.h"

namespace VXGI
{
	class IAreaLightTexture;
}

/**
 * A plane that emits light into the world through VXAL.
 */
UCLASS()
class ENGINE_API AAreaLightActor : public AStaticMeshActor
{
	GENERATED_UCLASS_BODY()
public:
	/** Base color of the light. Multiplied by diffuse and specular intensities, and by the texture. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	FLinearColor LightColor;

	/** Light intensity for diffuse illumination */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0"))
	float LightIntensityDiffuse;

	/** Light intensity for specular illumination */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0"))
	float LightIntensitySpecular;

	/** Texture that modulates the light color.
	  * NOTE: The texture MUST have a mip-map, otherwise the results will look like a warped projection of the texture onto surrounding objects. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
		class UTexture2D* LightTexture;

	/** Distance from the light plane where irradiance becomes zero.
	  * Such attenuation is not physically correct, but it improves performance by limiting the lights' volume of influence.
	  * 0 means no attenuation. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0"))
	float AttenuationRadius;

	/** Radial length of the region where irradiance is transitioned from physically correct values to zero */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0"))
	float TransitionLength;

	/** Light on-off switch. Does not affect the visibility of the light plane or its material. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableLight : 1;

	/** Enables shadowing of the area light */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableShadows : 1;

	/** Enables generation of shadows that reflect which part of the light's texture is occluded */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bTexturedShadows : 1;

	/** Enables screen-space contact shadows. Only effective when shadows in general are enabled. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableScreenSpaceShadows : 1;

	/** Overall quality of the shadows and, in some cases, texturing. Higher quality is achieved by tracing more cones. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Quality;

	/** Quality of the screen-space contact shadows. Higher quality is achieved by sampling the depth buffer more frequently. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenSpaceShadowQuality;

	/** Multiplier for the number of cones being traced towards a light. Does not affect the angle of the cones. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.25", ClampMax = "2.0"))
	float DirectionalSamplingRate;

	/** Maximum fraction of hemisphere around a surface covered by the area light when the number of cones stops to grow. 
	  * If the light covers a larger fraction of the hemisphere, the maximum number of cones is used, but they become wider.*/
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.125", ClampMax = "1.0"))
	float MaxProjectedArea;

	/** Distance from the area light surface when the occlusion cones stop, in multiples of the voxel sample size at the area light. 
	  * If the light or a surface behind it is voxelized, it will produce self-occlusion. Using a larger offset mitigates or prevents this self-occlusion. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "4.0", UIMax = "2.0"))
	float LightSurfaceOffset;

	/** Weight of occlusion and texture sampling data from the previous frame */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float TemporalWeight;

	/** Controls the tradeoff between noise in high-detail areas (0) and temporal trailing under dynamic lighting (1). 
	  * Not really effective when neighborhood color clamping is used. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TemporalDetailReconstruction;

	/** Controls whether neighborhood color clamping (NCC) should be used in the temporal reprojection filter.
	  * NCC greatly reduces trailing artifacts, but adds some noise in high-detail areas. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere)
	uint32 bEnableNeighborhoodColorClamping : 1;

	/** Controls the variance in color that's allowed during NCC */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.1", ClampMax = "100.0", UIMax = "5.0"))
	float NeighborhoodClampingWidth;

	/** Enables the use of a bicubic B-spline filter to sample the texture, instead of bilinear.
	  * Produces smoother lighting, especially fine specular reflections, at a small performance cost. */
	UPROPERTY(BlueprintReadWrite, Category = "Area Light", EditAnywhere, AdvancedDisplay)
	uint32 bHighQualityTextureFilter : 1;
};
