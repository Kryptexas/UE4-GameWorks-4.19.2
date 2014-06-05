// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "StaticMeshResources.h"
#include "../../Renderer/Private/ScenePrivate.h"

/*
TUniformBufferRef<FPrimitiveUniformShaderParameters> CreatePrimitiveUniformBufferImmediate(
	const FMatrix& LocalToWorld,
	FVector BoundsOrigin,
	FVector Bounds,
	float BoundsRadius,
	bool bReceivesDecals
	)
{
	check(IsInRenderingThread());
	return TUniformBufferRef<FPrimitiveUniformShaderParameters>::CreateUniformBufferImmediate(
		GetPrimitiveUniformShaderParameters(LocalToWorld, BoundsOrigin, BoundsOrigin, Bounds, BoundsRadius, bReceivesDecals),
		UniformBuffer_MultiFrame
		);
}
*/

FLightMapInteraction FLightMapInteraction::Texture(
	const class ULightMapTexture2D* const* InTextures,
	const ULightMapTexture2D* InSkyOcclusionTexture,
	const FVector4* InCoefficientScales,
	const FVector4* InCoefficientAdds,
	const FVector2D& InCoordinateScale,
	const FVector2D& InCoordinateBias,
	bool bAllowHighQualityLightMaps)
{
	FLightMapInteraction Result;
	Result.Type = LMIT_Texture;

	bool bUseHighQualityLightMaps = AllowHighQualityLightmaps();

#if ALLOW_LQ_LIGHTMAPS && ALLOW_HQ_LIGHTMAPS
	// however, if simple and directional are allowed, then we must use the value passed in,
	// and then cache the number as well
	bUseHighQualityLightMaps = bUseHighQualityLightMaps && bAllowHighQualityLightMaps;

	Result.bAllowHighQualityLightMaps = bUseHighQualityLightMaps;
	if (bAllowHighQualityLightMaps)
	{
		Result.NumLightmapCoefficients = NUM_HQ_LIGHTMAP_COEF;
	}
	else
	{
		Result.NumLightmapCoefficients = NUM_LQ_LIGHTMAP_COEF;
	}
#endif

	//copy over the appropriate textures and scales
	if (bUseHighQualityLightMaps)
	{
#if ALLOW_HQ_LIGHTMAPS
		Result.HighQualityTexture = InTextures[0];
		Result.SkyOcclusionTexture = InSkyOcclusionTexture;
		for(uint32 CoefficientIndex = 0;CoefficientIndex < NUM_HQ_LIGHTMAP_COEF;CoefficientIndex++)
		{
			Result.HighQualityCoefficientScales[CoefficientIndex] = InCoefficientScales[CoefficientIndex];
			Result.HighQualityCoefficientAdds[CoefficientIndex] = InCoefficientAdds[CoefficientIndex];
		}
#endif
	}

	// NOTE: In PC editor we cache both Simple and Directional textures as we may need to dynamically switch between them
	if( GIsEditor || !bUseHighQualityLightMaps )
	{
#if ALLOW_LQ_LIGHTMAPS
		Result.LowQualityTexture = InTextures[1];
		for(uint32 CoefficientIndex = 0;CoefficientIndex < NUM_LQ_LIGHTMAP_COEF;CoefficientIndex++)
		{
			Result.LowQualityCoefficientScales[CoefficientIndex] = InCoefficientScales[ LQ_LIGHTMAP_COEF_INDEX + CoefficientIndex ];
			Result.LowQualityCoefficientAdds[CoefficientIndex] = InCoefficientAdds[ LQ_LIGHTMAP_COEF_INDEX + CoefficientIndex ];
		}
#endif
	}

	Result.CoordinateScale = InCoordinateScale;
	Result.CoordinateBias = InCoordinateBias;
	return Result;
}

float ComputeBoundsScreenSize( const FVector4& Origin, const float SphereRadius, const FSceneView& View )
{
	// Only need one component from a view transformation; just calculate the one we're interested in.
	const float Divisor =  Dot3(Origin - View.ViewMatrices.ViewOrigin, View.ViewMatrices.ViewMatrix.GetColumn(2));

	// Get projection multiple accounting for view scaling.
	const float ScreenMultiple = FMath::Max(View.ViewRect.Width() / 2.0f * View.ViewMatrices.ProjMatrix.M[0][0],
		View.ViewRect.Height() / 2.0f * View.ViewMatrices.ProjMatrix.M[1][1]);

	const float ScreenRadius = ScreenMultiple * SphereRadius / FMath::Max(Divisor, 1.0f);
	const float ScreenArea = PI * ScreenRadius * ScreenRadius;
	return FMath::Clamp(ScreenArea / View.ViewRect.Area(), 0.0f, 1.0f);
}

int8 ComputeStaticMeshLOD( const FStaticMeshRenderData* RenderData, const FVector4& Origin, const float SphereRadius, const FSceneView& View, float FactorScale )
{
	const int32 NumLODs = MAX_STATIC_MESH_LODS;

	const float ScreenSize = ComputeBoundsScreenSize(Origin, SphereRadius, View) * FactorScale;

	// Walk backwards and return the first matching LOD
	for(int32 LODIndex = NumLODs - 1 ; LODIndex >= 0 ; --LODIndex)
	{
		if(RenderData->ScreenSize[LODIndex] > ScreenSize)
		{
			return LODIndex;
		}
	}

	return 0;
}

int8 ComputeLODForMeshes( const TIndirectArray<class FStaticMesh>& StaticMeshes, FSceneView& View, const FVector4& Origin, float SphereRadius, int32 ForcedLODLevel, float ScreenSizeScale )
{
	int8 LODToRender = 0;

	// Handle forced LOD level first
	if(ForcedLODLevel >= 0)
	{
		int8 MaxLOD = 0;
		for(int32 MeshIndex = 0 ; MeshIndex < StaticMeshes.Num() ; ++MeshIndex)
		{
			const FStaticMesh&  Mesh = StaticMeshes[MeshIndex];
			MaxLOD = FMath::Max(MaxLOD, Mesh.LODIndex);
		}
		LODToRender = FMath::Clamp<int8>(ForcedLODLevel, 0, MaxLOD);
	}
	else
	{
		int32 NumMeshes = StaticMeshes.Num();

		const float ScreenSize = ComputeBoundsScreenSize(Origin, SphereRadius, View);

		for(int32 MeshIndex = NumMeshes-1 ; MeshIndex >= 0 ; --MeshIndex)
		{
			const FStaticMesh& Mesh = StaticMeshes[MeshIndex];

			if(View.Family->EngineShowFlags.LOD == 1)
			{
				float MeshScreenSize = Mesh.ScreenSize * ScreenSizeScale;

				if(MeshScreenSize >= ScreenSize)
				{
					LODToRender = Mesh.LODIndex;
					break;
				}
			}
		}
	}
	return LODToRender;
}
