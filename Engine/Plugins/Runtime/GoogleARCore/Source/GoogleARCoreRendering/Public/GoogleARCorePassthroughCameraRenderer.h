// Copyright 2017 Google Inc.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ConstructorHelpers.h"
#include "RHI.h"
#include "RenderResource.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture.h"

#include "GoogleARCorePassthroughCameraRenderer.generated.h"

/** A helper class that is used to load the GoogleARCorePassthroughCameraMaterial from its default object. */
UCLASS()
class GOOGLEARCORERENDERING_API UGoogleARCoreCameraOverlayMaterialLoader : public UObject
{
	GENERATED_BODY()

public:
	/** A pointer to the camera overlay material that will be used to render the passthrough camera texture as background. */
	UPROPERTY()
		UMaterialInterface* DefaultCameraOverlayMaterial;

	UGoogleARCoreCameraOverlayMaterialLoader()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultOverlayMaterialRef(TEXT("/GoogleARCore/GoogleARCorePassthroughCameraMaterial.GoogleARCorePassthroughCameraMaterial"));
		DefaultCameraOverlayMaterial = DefaultOverlayMaterialRef.Object;

		UMaterialInstanceDynamic *DynMat = UMaterialInstanceDynamic::Create(
			DefaultOverlayMaterialRef.Object, this, FName("GoogleARCorePassthroughCameraMaterial_Dynamic"));

		DefaultCameraOverlayMaterial = DynMat;
	}
};

class FRHICommandListImmediate;

class GOOGLEARCORERENDERING_API FGoogleARCorePassthroughCameraRenderer
{
public:
	FGoogleARCorePassthroughCameraRenderer();

	void SetDefaultCameraOverlayMaterial(UMaterialInterface* InDefaultCameraOverlayMaterial);

	void InitializeOverlayMaterial();

	void SetOverlayMaterialInstance(UMaterialInterface* NewMaterialInstance);
	void ResetOverlayMaterialToDefault();

	void InitializeRenderer_RenderThread(FTextureRHIRef ExternalTexture);

	void UpdateOverlayUVCoordinate_RenderThread(TArray<float>& InOverlayUVs);
	void RenderVideoOverlay_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView);
	void CopyVideoTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef DstTexture, FIntPoint TargetSize);

	const TArray<float> OverlayQuadUVs;
private:
	bool bInitialized;
	FIndexBufferRHIRef OverlayIndexBufferRHI;
	FVertexBufferRHIRef OverlayVertexBufferRHI;
	FVertexBufferRHIRef OverlayCopyVertexBufferRHI;
	FTextureRHIRef VideoTexture;
	float OverlayTextureUVs[8];
	bool bMaterialInitialized;
	UMaterialInterface* DefaultOverlayMaterial;
	UMaterialInterface* OverrideOverlayMaterial;
	UMaterialInterface* RenderingOverlayMaterial;
};

