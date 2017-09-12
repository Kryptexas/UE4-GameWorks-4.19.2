// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "AppleARKitVideoOverlay.generated.h"

/** Helper class to ensure the ARKit camera material is cooked. */
UCLASS()
class UARKitCameraOverlayMaterialLoader : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	UMaterialInterface* DefaultCameraOverlayMaterial;
	
	UARKitCameraOverlayMaterialLoader()
	{
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultOverlayMaterialRef(TEXT("/AppleARKit/ARKitCameraMaterial.ARKitCameraMaterial"));
		DefaultCameraOverlayMaterial = DefaultOverlayMaterialRef.Object;
	}
};

struct FAppleARKitFrame;

class FAppleARKitVideoOverlay
{
public:
	FAppleARKitVideoOverlay();

	void UpdateVideoTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FAppleARKitFrame& Frame);
	void RenderVideoOverlay_RenderThread(FRHICommandListImmediate& RHICmdList, const FSceneView& InView);

private:
	FTextureRHIRef VideoTextureY;
	FTextureRHIRef VideoTextureCbCr;
	UMaterialInterface* RenderingOverlayMaterial;
	FIndexBufferRHIRef OverlayIndexBufferRHI;
	FVertexBufferRHIRef OverlayVertexBufferRHI;
	double LastUpdateTimestamp;
};
