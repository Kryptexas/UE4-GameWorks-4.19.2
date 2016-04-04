// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PlanarReflectionSceneProxy.h: 
=============================================================================*/

#pragma once

class FPlanarReflectionRenderTarget : public FTexture, public FRenderTarget
{
public:

	FPlanarReflectionRenderTarget(FIntPoint InSize) :
		Size(InSize)
	{}

	virtual const FTexture2DRHIRef& GetRenderTargetTexture() const 
	{
		return (const FTexture2DRHIRef&)TextureRHI;
	}

	virtual void InitDynamicRHI()
	{
		// Create the sampler state RHI resource.
		FSamplerStateInitializerRHI SamplerStateInitializer
		(
			SF_Bilinear,
			AM_Clamp,
			AM_Clamp,
			AM_Clamp
		);
		SamplerStateRHI = RHICreateSamplerState(SamplerStateInitializer);

		FTexture2DRHIRef Texture2DRHI;
		FRHIResourceCreateInfo CreateInfo;

		RHICreateTargetableShaderResource2D(
			GetSizeX(), 
			GetSizeY(), 
			PF_FloatRGBA,
			1,
			0,
			TexCreate_RenderTargetable,
			false,
			CreateInfo,
			RenderTargetTextureRHI,
			Texture2DRHI
			);
		TextureRHI = (FTextureRHIRef&)Texture2DRHI;
	}

	virtual FIntPoint GetSizeXY() const { return Size; }

	/** Returns the width of the texture in pixels. */
	virtual uint32 GetSizeX() const
	{
		return Size.X;
	}
	/** Returns the height of the texture in pixels. */
	virtual uint32 GetSizeY() const
	{
		return Size.Y;
	}

	virtual float GetDisplayGamma() const { return 1.0f; }

	virtual FString GetFriendlyName() const override { return TEXT("FPlanarReflectionRenderTarget"); }

private:

	FIntPoint Size;
};

class FPlanarReflectionSceneProxy
{
public:

	FPlanarReflectionSceneProxy(UPlanarReflectionComponent* Component, FPlanarReflectionRenderTarget* InRenderTarget);

	void UpdateTransform(const FMatrix& NewTransform)
	{
		ReflectionPlane = FPlane(NewTransform.TransformPosition(FVector::ZeroVector), NewTransform.TransformVector(FVector(0, 0, 1)));

		const FMirrorMatrix MirrorMatrix(ReflectionPlane);
		// Using TransposeAdjoint instead of full inverse because we only care about transforming normals
		const FMatrix InverseTransposeMirrorMatrix4x4 = MirrorMatrix.TransposeAdjoint();
		InverseTransposeMirrorMatrix4x4.GetScaledAxes((FVector&)InverseTransposeMirrorMatrix[0], (FVector&)InverseTransposeMirrorMatrix[1], (FVector&)InverseTransposeMirrorMatrix[2]);
	}

	FPlane ReflectionPlane;
	FVector PlanarReflectionParameters;
	FVector2D PlanarReflectionParameters2;
	FMatrix ProjectionWithExtraFOV;
	FVector4 InverseTransposeMirrorMatrix[3];
	FPlanarReflectionRenderTarget* RenderTarget;
	FName OwnerName;
};