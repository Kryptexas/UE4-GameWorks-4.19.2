/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "CoreMinimal.h"
#include "RHI.h"
#include "RenderResource.h"
#include "ShaderParameters.h"
#include "Shader.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"


class FRHICommandListImmediate;

class TANGORENDERER_API FTangoVideoOverlayRendererRHI
{
public:
	FTangoVideoOverlayRendererRHI();

	void SetDefaultCameraOverlayMaterial(UMaterialInterface* InDefaultCameraOverlayMaterial);

	void InitializeOverlayMaterial();

	void SetOverlayMaterialInstance(UMaterialInterface* NewMaterialInstance);
	void ResetOverlayMaterialToDefault();

	void InitializeIndexBuffer_RenderThread();
	uint32 AllocateVideoTexture_RenderThread();
	void UpdateOverlayUVCoordinate_RenderThread(TArray<float>& InOverlayUVs);
	void RenderVideoOverlay_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView);
	void CopyVideoTexture_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRHIParamRef DstTexture, FIntPoint TargetSize);

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

