// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TranslucentRendering.h: Translucent rendering definitions.
=============================================================================*/

#pragma once

/**
* Translucent draw policy factory.
* Creates the policies needed for rendering a mesh based on its material
*/
class FTranslucencyDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = true };
	struct ContextType 
	{
		const FProjectedShadowInfo* TranslucentSelfShadow;

		ContextType(const FProjectedShadowInfo* InTranslucentSelfShadow = NULL) 
			: TranslucentSelfShadow(InTranslucentSelfShadow) 
		{}
	};

	/**
	* Render a dynamic mesh using a translucent draw policy 
	* @return true if the mesh rendered
	*/
	static bool DrawDynamicMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	/**
	* Render a dynamic mesh using a translucent draw policy 
	* @return true if the mesh rendered
	*/
	static bool DrawStaticMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FStaticMesh& StaticMesh,
		const uint64& BatchElementMask,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		// Note: blend mode does not depend on the feature level.
		return !IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode());
	}


private:
	/**
	* Render a dynamic or static mesh using a translucent draw policy
	* @return true if the mesh rendered
	*/
	static bool DrawMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		const uint64& BatchElementMask,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);
};


/**
* Translucent draw policy factory.
* Creates the policies needed for rendering a mesh based on its material
*/
class FTranslucencyForwardShadingDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = true };
	struct ContextType 
	{
	};

	/**
	* Render a dynamic mesh using a translucent draw policy 
	* @return true if the mesh rendered
	*/
	static bool DrawDynamicMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	/**
	* Render a dynamic mesh using a translucent draw policy 
	* @return true if the mesh rendered
	*/
	static bool DrawStaticMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FStaticMesh& StaticMesh,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
	{
		// Note: blend mode does not depend on the feature level.
		return !IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode());
	}
};

/** Represents a subregion of a volume texture. */
struct FVolumeBounds
{
	int32 MinX, MinY, MinZ;
	int32 MaxX, MaxY, MaxZ;

	FVolumeBounds() :
		MinX(0),
		MinY(0),
		MinZ(0),
		MaxX(0),
		MaxY(0),
		MaxZ(0)
	{}

	FVolumeBounds(int32 Max) :
		MinX(0),
		MinY(0),
		MinZ(0),
		MaxX(Max),
		MaxY(Max),
		MaxZ(Max)
	{}

	bool IsValid() const
	{
		return MaxX > MinX && MaxY > MinY && MaxZ > MinZ;
	}
};

/** Vertex shader used to write to a range of slices of a 3d volume texture. */
class FWriteToSliceVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWriteToSliceVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4); 
	}

	FWriteToSliceVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		UVScaleBias.Bind(Initializer.ParameterMap, TEXT("UVScaleBias"));
	}

	FWriteToSliceVS() {}

	void SetParameters(const FVolumeBounds& VolumeBounds, uint32 VolumeResolution)
	{
		const float InvVolumeResolution = 1.0f / VolumeResolution;
		SetShaderValue(GetVertexShader(), UVScaleBias, FVector4(
			(VolumeBounds.MaxX - VolumeBounds.MinX) * InvVolumeResolution, 
			(VolumeBounds.MaxY - VolumeBounds.MinY) * InvVolumeResolution,
			VolumeBounds.MinX * InvVolumeResolution,
			VolumeBounds.MinY * InvVolumeResolution));
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << UVScaleBias;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter UVScaleBias;
};

/** Geometry shader used to write to a range of slices of a 3d volume texture. */
class FWriteToSliceGS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FWriteToSliceGS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4); 
	}

	FWriteToSliceGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		MinZ.Bind(Initializer.ParameterMap, TEXT("MinZ"));
	}
	FWriteToSliceGS() {}

	void SetParameters(const FVolumeBounds& VolumeBounds)
	{
		SetShaderValue(GetGeometryShader(), MinZ, VolumeBounds.MinZ);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << MinZ;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter MinZ;
};

extern void RasterizeToVolumeTexture(FVolumeBounds VolumeBounds);