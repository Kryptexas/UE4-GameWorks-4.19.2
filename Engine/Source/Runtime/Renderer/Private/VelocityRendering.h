// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VelocityRendering.h: Velocity rendering definitions.
=============================================================================*/

#pragma once

/**
 * Outputs a 2d velocity vector.
 */
class FVelocityDrawingPolicy : public FMeshDrawingPolicy
{
public:
	FVelocityDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource
		);

	// FMeshDrawingPolicy interface.
	bool Matches(const FVelocityDrawingPolicy& Other) const
	{
		return FMeshDrawingPolicy::Matches(Other)
			&& HullShader == Other.HullShader
			&& DomainShader == Other.DomainShader
			&& VertexShader == Other.VertexShader
			&& PixelShader == Other.PixelShader;
	}
	void DrawShared( const FSceneView* View, FBoundShaderStateRHIRef ShaderState ) const;
	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData
		) const;

	FBoundShaderStateRHIRef CreateBoundShaderState();

	friend int32 Compare(const FVelocityDrawingPolicy& A, const FVelocityDrawingPolicy& B);

	bool SupportsVelocity( ) const;

	/** Determines whether this primitive has motionblur velocity to render */
	static bool HasVelocity(const FViewInfo& View, const FPrimitiveSceneInfo* PrimitiveSceneInfo);

private:
	class FVelocityVS* VertexShader;
	class FVelocityPS* PixelShader;
	class FVelocityHS* HullShader;
	class FVelocityDS* DomainShader;
};

/**
 * A drawing policy factory for rendering motion velocity.
 */
class FVelocityDrawingPolicyFactory : public FDepthDrawingPolicyFactory
{
public:
	static void AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh, ContextType = ContextType(DDM_AllOccluders));
	static bool DrawDynamicMesh(	
		const FViewInfo& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);
};

/** Get the cvar clamped state */
int32 GetMotionBlurQualityFromCVar();

/** If this view need motion blur processing */
bool IsMotionBlurEnabled(const FViewInfo& View);
