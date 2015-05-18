// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GlobalDistanceFieldParameters.h
=============================================================================*/

#pragma once

/** Must match global distance field shaders. */
const int32 GMaxGlobalDistanceFieldClipmaps = 4;

class FGlobalDistanceFieldParameterData
{
public:

	FGlobalDistanceFieldParameterData()
	{
		FPlatformMemory::Memzero(this, sizeof(FGlobalDistanceFieldParameterData));
	}

	FVector4 CenterAndExtent[GMaxGlobalDistanceFieldClipmaps];
	FVector4 WorldToUVAddAndMul[GMaxGlobalDistanceFieldClipmaps];
	FTextureRHIParamRef Textures[GMaxGlobalDistanceFieldClipmaps];
	float GlobalDFResolution;
	float MaxDistance;
};

class FGlobalDistanceFieldParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		GlobalDistanceFieldTexture.Bind(ParameterMap,TEXT("GlobalDistanceFieldTexture"));
		GlobalDistanceFieldSampler.Bind(ParameterMap,TEXT("GlobalDistanceFieldSampler"));
		GlobalVolumeCenterAndExtent.Bind(ParameterMap,TEXT("GlobalVolumeCenterAndExtent"));
		GlobalVolumeWorldToUVAddAndMul.Bind(ParameterMap,TEXT("GlobalVolumeWorldToUVAddAndMul"));
		GlobalVolumeDimension.Bind(ParameterMap,TEXT("GlobalVolumeDimension"));
		GlobalVolumeTexelSize.Bind(ParameterMap,TEXT("GlobalVolumeTexelSize"));
		MaxGlobalDistance.Bind(ParameterMap,TEXT("MaxGlobalDistance"));
	}

	friend FArchive& operator<<(FArchive& Ar,FGlobalDistanceFieldParameters& Parameters)
	{
		Ar << Parameters.GlobalDistanceFieldTexture;
		Ar << Parameters.GlobalDistanceFieldSampler;
		Ar << Parameters.GlobalVolumeCenterAndExtent;
		Ar << Parameters.GlobalVolumeWorldToUVAddAndMul;
		Ar << Parameters.GlobalVolumeDimension;
		Ar << Parameters.GlobalVolumeTexelSize;
		Ar << Parameters.MaxGlobalDistance;
		return Ar;
	}

	template<typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef ShaderRHI, const FGlobalDistanceFieldParameterData& ParameterData)
	{
		if (GlobalVolumeCenterAndExtent.IsBound() || GlobalVolumeWorldToUVAddAndMul.IsBound())
		{
			for (int32 ClipmapIndex = 0; ClipmapIndex < (int32)GlobalDistanceFieldTexture.GetNumResources(); ClipmapIndex++)
			{
				SetTextureParameter(
					RHICmdList,
					ShaderRHI,
					GlobalDistanceFieldTexture,
					GlobalDistanceFieldSampler,
					TStaticSamplerState<SF_Bilinear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
					ParameterData.Textures[ClipmapIndex],
					ClipmapIndex);
			}

			SetShaderValueArray(RHICmdList, ShaderRHI, GlobalVolumeCenterAndExtent, ParameterData.CenterAndExtent, GMaxGlobalDistanceFieldClipmaps);
			SetShaderValueArray(RHICmdList, ShaderRHI, GlobalVolumeWorldToUVAddAndMul, ParameterData.WorldToUVAddAndMul, GMaxGlobalDistanceFieldClipmaps);
			SetShaderValue(RHICmdList, ShaderRHI, GlobalVolumeDimension, ParameterData.GlobalDFResolution);
			SetShaderValue(RHICmdList, ShaderRHI, GlobalVolumeTexelSize, 1.0f / ParameterData.GlobalDFResolution);
			SetShaderValue(RHICmdList, ShaderRHI, MaxGlobalDistance, ParameterData.MaxDistance);
		}
	}

private:
	FShaderResourceParameter GlobalDistanceFieldTexture;
	FShaderResourceParameter GlobalDistanceFieldSampler;
	FShaderParameter GlobalVolumeCenterAndExtent;
	FShaderParameter GlobalVolumeWorldToUVAddAndMul;
	FShaderParameter GlobalVolumeDimension;
	FShaderParameter GlobalVolumeTexelSize;
	FShaderParameter MaxGlobalDistance;
};
