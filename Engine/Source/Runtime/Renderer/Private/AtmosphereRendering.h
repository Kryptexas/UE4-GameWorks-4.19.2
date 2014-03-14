// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AtmosphereRendering.h: Fog rendering
=============================================================================*/

#pragma once

namespace EAtmosphereRenderFlag
{
	enum Type
	{
		E_EnableAll = 0,
		E_DisableSunDisk = 1,
		E_DisableGroundScattering = 2,
		E_DisableLightShaft = 4, // Light Shaft shadow
		E_DisableSunAndGround = E_DisableSunDisk | E_DisableGroundScattering,
		E_DisableSunAndLightShaft = E_DisableSunDisk | E_DisableLightShaft,
		E_DisableGroundAndLightShaft = E_DisableGroundScattering | E_DisableLightShaft,
		E_DisableAll = E_DisableSunDisk | E_DisableGroundScattering | E_DisableLightShaft,
		E_RenderFlagMax = E_DisableAll + 1,
		E_LightShaftMask = (~E_DisableLightShaft),
	};
}

/** The properties of a atmospheric fog layer which are used for rendering. */
class FAtmosphericFogSceneInfo
{
public:
	/** The fog component the scene info is for. */
	const UAtmosphericFogComponent* Component;
	float SunMultiplier;
	float FogMultiplier;
	float InvDensityMultiplier;
	float DensityOffset;
	float GroundOffset;
	float DistanceScale;
	float AltitudeScale;
	float RHeight;
	float StartDistance;
	float DistanceOffset;
	FLinearColor DefaultSunColor;
	FVector DefaultSunDirection;
	uint32 RenderFlag;
	uint32 InscatterAltitudeSampleNum;

#if WITH_EDITORONLY_DATA
	/** Atmosphere pre-computation related data */
	bool bNeedRecompute;
	int32 MaxScatteringOrder;
	int32 AtmospherePhase;
	int32 Atmosphere3DTextureIndex;
	int32 AtmoshpereOrder;
	class FAtmosphereTextures* AtmosphereTextures;
	class FThreadSafeCounter& PrecomputeCounter;
	FByteBulkData PrecomputeTransmittance;
	FByteBulkData PrecomputeIrradiance;
	FByteBulkData PrecomputeInscatter;
#endif

	/** Initialization constructor. */
	explicit FAtmosphericFogSceneInfo(UAtmosphericFogComponent* InComponent, const FScene* InScene);
	~FAtmosphericFogSceneInfo();

#if WITH_EDITOR
	void PrecomputeTextures(const FViewInfo* View, FSceneViewFamily* ViewFamily);
	void StartPrecompute();

private:
	/** Atmosphere pre-computation related functions */
	FIntPoint GetTextureSize();
	inline void DrawQuad(const FIntRect& ViewRect);
	void GetLayerValue(int Layer, float& AtmosphereR, FVector4& DhdH);
	void RenderAtmosphereShaders(const FViewInfo& View, const FIntRect& ViewRect);
	void PrecomputeAtmosphereData(const FViewInfo* View, FSceneViewFamily& ViewFamily);

	void ReadPixelsPtr(TRefCountPtr<IPooledRenderTarget> RenderTarget, FColor* OutData, FIntRect InRect);
	void Read3DPixelsPtr(TRefCountPtr<IPooledRenderTarget> RenderTarget, FFloat16Color* OutData, FIntRect InRect, FIntPoint InZMinMax);
#endif
};

/** Shader parameters needed for atmosphere passes. */
class FAtmosphereShaderTextureParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		TransmittanceTexture.Bind(ParameterMap,TEXT("AtmosphereTransmittanceTexture"));
		TransmittanceTextureSampler.Bind(ParameterMap,TEXT("AtmosphereTransmittanceTextureSampler"));
		IrradianceTexture.Bind(ParameterMap,TEXT("AtmosphereIrradianceTexture"));
		IrradianceTextureSampler.Bind(ParameterMap,TEXT("AtmosphereIrradianceTextureSampler"));
		InscatterTexture.Bind(ParameterMap,TEXT("AtmosphereInscatterTexture"));
		InscatterTextureSampler.Bind(ParameterMap,TEXT("AtmosphereInscatterTextureSampler"));
	}

	template<typename ShaderRHIParamRef>
	void Set(const ShaderRHIParamRef ShaderRHI, const FSceneView& View) const
	{
		if (TransmittanceTexture.IsBound() || IrradianceTexture.IsBound() || InscatterTexture.IsBound())
		{
			SetTextureParameter(ShaderRHI, TransmittanceTexture, TransmittanceTextureSampler, 
				TStaticSamplerState<SF_Bilinear>::GetRHI(), View.AtmosphereTransmittanceTexture);
			SetTextureParameter(ShaderRHI, IrradianceTexture, IrradianceTextureSampler, 
				TStaticSamplerState<SF_Bilinear>::GetRHI(), View.AtmosphereIrradianceTexture);
			SetTextureParameter(ShaderRHI, InscatterTexture, InscatterTextureSampler, 
				TStaticSamplerState<SF_Bilinear>::GetRHI(), View.AtmosphereInscatterTexture);
		}
	}

	friend FArchive& operator<<(FArchive& Ar,FAtmosphereShaderTextureParameters& P);

private:
	FShaderResourceParameter TransmittanceTexture;
	FShaderResourceParameter TransmittanceTextureSampler;
	FShaderResourceParameter IrradianceTexture;
	FShaderResourceParameter IrradianceTextureSampler;
	FShaderResourceParameter InscatterTexture;
	FShaderResourceParameter InscatterTextureSampler;
};

extern bool ShouldRenderAtmosphere(const FSceneViewFamily& Family);