// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GlobalDistanceField.h
=============================================================================*/

#pragma once

class FGlobalDistanceFieldParameters
{
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		GlobalDistanceFieldTexture.Bind(ParameterMap,TEXT("GlobalDistanceFieldTexture"));
		GlobalDistanceFieldSampler.Bind(ParameterMap,TEXT("GlobalDistanceFieldSampler"));
		GlobalVolumeCenterAndExtent.Bind(ParameterMap,TEXT("GlobalVolumeCenterAndExtent"));
		GlobalVolumeDimension.Bind(ParameterMap,TEXT("GlobalVolumeDimension"));
		GlobalVolumeWorldToUVAddAndMul.Bind(ParameterMap,TEXT("GlobalVolumeWorldToUVAddAndMul"));
	}

	friend FArchive& operator<<(FArchive& Ar,FGlobalDistanceFieldParameters& Parameters)
	{
		Ar << Parameters.GlobalDistanceFieldTexture;
		Ar << Parameters.GlobalDistanceFieldSampler;
		Ar << Parameters.GlobalVolumeCenterAndExtent;
		Ar << Parameters.GlobalVolumeDimension;
		Ar << Parameters.GlobalVolumeWorldToUVAddAndMul;
		return Ar;
	}

	template<typename ShaderRHIParamRef>
	void Set(FRHICommandList& RHICmdList, const ShaderRHIParamRef ShaderRHI, const FGlobalDistanceFieldInfo& Info)
	{
		for (int32 ClipmapIndex = 0; ClipmapIndex < (int32)GlobalDistanceFieldTexture.GetNumResources(); ClipmapIndex++)
		{
			FTextureRHIParamRef TextureValue = ClipmapIndex < Info.Clipmaps.Num() 
				? Info.Clipmaps[ClipmapIndex].RenderTarget->GetRenderTargetItem().ShaderResourceTexture 
				: NULL;

			SetTextureParameter(
				RHICmdList,
				ShaderRHI,
				GlobalDistanceFieldTexture,
				GlobalDistanceFieldSampler,
				TStaticSamplerState<SF_Bilinear,AM_Wrap,AM_Wrap,AM_Wrap>::GetRHI(),
				TextureValue,
				ClipmapIndex
				);
		}

		FVector4 CenterAndExtent[GMaxGlobalDistanceFieldClipmaps];
		FVector4 WorldToUVAddAndMul[GMaxGlobalDistanceFieldClipmaps];

		const int32 NumToSet = GlobalVolumeCenterAndExtent.GetNumBytes() / sizeof(FVector4);

		for (int32 ClipmapIndex = 0; ClipmapIndex < NumToSet; ClipmapIndex++)
		{
			if (ClipmapIndex < Info.Clipmaps.Num())
			{
				const FGlobalDistanceFieldClipmap& Clipmap = Info.Clipmaps[ClipmapIndex];
				CenterAndExtent[ClipmapIndex] = FVector4(Clipmap.Bounds.GetCenter(), Clipmap.Bounds.GetExtent().X);

				// GlobalUV = (WorldPosition - GlobalVolumeCenterAndExtent[ClipmapIndex].xyz + GlobalVolumeScollOffset[ClipmapIndex].xyz) / (GlobalVolumeCenterAndExtent[ClipmapIndex].w * 2) + .5f;
				// WorldToUVMul = 1.0f / (GlobalVolumeCenterAndExtent[ClipmapIndex].w * 2)
				// WorldToUVAdd = (GlobalVolumeScollOffset[ClipmapIndex].xyz - GlobalVolumeCenterAndExtent[ClipmapIndex].xyz) / (GlobalVolumeCenterAndExtent[ClipmapIndex].w * 2) + .5f
				const FVector WorldToUVAdd = (Clipmap.ScrollOffset - Clipmap.Bounds.GetCenter()) / (Clipmap.Bounds.GetExtent().X * 2) + FVector(.5f);
				WorldToUVAddAndMul[ClipmapIndex] = FVector4(WorldToUVAdd, 1.0f / (Clipmap.Bounds.GetExtent().X * 2));
			}
			else
			{
				CenterAndExtent[ClipmapIndex] = FVector4(0);
				WorldToUVAddAndMul[ClipmapIndex] = FVector4(0);
			}
		}

		SetShaderValueArray(RHICmdList, ShaderRHI, GlobalVolumeCenterAndExtent, CenterAndExtent, GMaxGlobalDistanceFieldClipmaps);
		SetShaderValueArray(RHICmdList, ShaderRHI, GlobalVolumeWorldToUVAddAndMul, WorldToUVAddAndMul, GMaxGlobalDistanceFieldClipmaps);

		extern int32 GAOGlobalDFResolution;
		SetShaderValue(RHICmdList, ShaderRHI, GlobalVolumeDimension, (float)GAOGlobalDFResolution);
	}

private:
	FShaderResourceParameter GlobalDistanceFieldTexture;
	FShaderResourceParameter GlobalDistanceFieldSampler;
	FShaderParameter GlobalVolumeCenterAndExtent;
	FShaderParameter GlobalVolumeDimension;
	FShaderParameter GlobalVolumeScollOffset;
	FShaderParameter GlobalVolumeWorldToUVAddAndMul;
};

extern int32 GAOGlobalDistanceField;
extern int32 GAOVisualizeGlobalDistanceField;

inline bool UseGlobalDistanceField()
{
	return GAOGlobalDistanceField != 0;
}

inline bool UseGlobalDistanceField(const FDistanceFieldAOParameters& Parameters)
{
	return UseGlobalDistanceField() && Parameters.GlobalMaxOcclusionDistance > 0;
}

/** 
 * Updates the global distance field for a view.  
 * Typically issues updates for just the newly exposed regions of the volume due to camera movement.
 * In the worst case of a camera cut or large distance field scene changes, a full update of the global distance field will be done.
 **/
extern void UpdateGlobalDistanceFieldVolume(
	FRHICommandListImmediate& RHICmdList, 
	const FViewInfo& View, 
	const FScene* Scene, 
	float MaxOcclusionDistance, 
	FGlobalDistanceFieldInfo& Info);