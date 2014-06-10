// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StaticMeshResources.h"

class FVertexFactoryShaderParameters;

//////////////////////////////////////////////////////////////////////////
// SplineMeshVertexFactory

/** A vertex factory for spline-deformed static meshes */
struct FSplineMeshVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSplineMeshVertexFactory);
public:

	FSplineMeshVertexFactory(class FSplineMeshSceneProxy* InSplineProxy) :
		SplineSceneProxy(InSplineProxy)
	{
	}

	/** Should we cache the material's shadertype on this platform with this vertex factory? */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return (Material->IsUsedWithSplineMeshes() || Material->IsSpecialEngineMaterial())
			&& FLocalVertexFactory::ShouldCache(Platform, Material, ShaderType)
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	/** Modify compile environment to enable spline deformation */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLocalVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_SPLINEDEFORM"), TEXT("1"));
	}

	/** Copy the data from another vertex factory */
	void Copy(const FSplineMeshVertexFactory& Other)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSplineMeshVertexFactoryCopyData,
			FSplineMeshVertexFactory*, VertexFactory, this,
			const DataType*, DataCopy, &Other.Data,
			{
			VertexFactory->Data = *DataCopy;
		});
		BeginUpdateResourceRHI(this);
	}

	// FRenderResource interface.
	virtual void InitRHI()
	{
		FLocalVertexFactory::InitRHI();
	}

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

	class FSplineMeshSceneProxy* SplineSceneProxy;

private:
};

//////////////////////////////////////////////////////////////////////////
// FSplineMeshVertexFactoryShaderParameters

/** Factory specific params */
class FSplineMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	void Bind(const FShaderParameterMap& ParameterMap) OVERRIDE;

	void SetMesh(FRHICommandList& RHICmdList, FShader* Shader, const FVertexFactory* VertexFactory, const FSceneView& View, const FMeshBatchElement& BatchElement, uint32 DataFlags) const OVERRIDE;

	void Serialize(FArchive& Ar) OVERRIDE
	{
		Ar << SplineStartPosParam;
		Ar << SplineStartTangentParam;
		Ar << SplineStartRollParam;
		Ar << SplineStartScaleParam;
		Ar << SplineStartOffsetParam;

		Ar << SplineEndPosParam;
		Ar << SplineEndTangentParam;
		Ar << SplineEndRollParam;
		Ar << SplineEndScaleParam;
		Ar << SplineEndOffsetParam;

		Ar << SplineUpDirParam;
		Ar << SmoothInterpRollScaleParam;

		Ar << SplineMeshMinZParam;
		Ar << SplineMeshScaleZParam;

		Ar << SplineMeshDirParam;
		Ar << SplineMeshXParam;
		Ar << SplineMeshYParam;
	}

	virtual uint32 GetSize() const OVERRIDE
	{
		return sizeof(*this);
	}

private:
	FShaderParameter SplineStartPosParam;
	FShaderParameter SplineStartTangentParam;
	FShaderParameter SplineStartRollParam;
	FShaderParameter SplineStartScaleParam;
	FShaderParameter SplineStartOffsetParam;

	FShaderParameter SplineEndPosParam;
	FShaderParameter SplineEndTangentParam;
	FShaderParameter SplineEndRollParam;
	FShaderParameter SplineEndScaleParam;
	FShaderParameter SplineEndOffsetParam;

	FShaderParameter SplineUpDirParam;
	FShaderParameter SmoothInterpRollScaleParam;

	FShaderParameter SplineMeshMinZParam;
	FShaderParameter SplineMeshScaleZParam;

	FShaderParameter SplineMeshDirParam;
	FShaderParameter SplineMeshXParam;
	FShaderParameter SplineMeshYParam;
};

//////////////////////////////////////////////////////////////////////////
// SplineMeshSceneProxy

/** Scene proxy for SplineMesh instance */
class FSplineMeshSceneProxy : public FStaticMeshSceneProxy
{
protected:
	struct FLODResources
	{
		/** Pointer to vertex factory object */
		FSplineMeshVertexFactory* VertexFactory;

		FLODResources(FSplineMeshVertexFactory* InVertexFactory) :
			VertexFactory(InVertexFactory)
		{
		}
	};
public:

	FSplineMeshSceneProxy(USplineMeshComponent* InComponent) :
		FStaticMeshSceneProxy(InComponent)
	{
		// make sure all the materials are okay to be rendered as a spline mesh
		for (FStaticMeshSceneProxy::FLODInfo& LODInfo : LODs)
		{
			for (FStaticMeshSceneProxy::FLODInfo::FSectionInfo& Section : LODInfo.Sections)
			{
				if (!Section.Material->CheckMaterialUsage(MATUSAGE_SplineMesh))
				{
					Section.Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}
			}
		}

		// Copy spline params from component
		SplineParams = InComponent->SplineParams;
		SplineUpDir = InComponent->SplineUpDir;
		bSmoothInterpRollScale = InComponent->bSmoothInterpRollScale;
		ForwardAxis = InComponent->ForwardAxis;

		// Fill in info about the mesh
		FBoxSphereBounds StaticMeshBounds = StaticMesh->GetBounds();
		SplineMeshScaleZ = 0.5f / USplineMeshComponent::GetAxisValue(StaticMeshBounds.BoxExtent, ForwardAxis); // 1/(2 * Extent)
		SplineMeshMinZ = USplineMeshComponent::GetAxisValue(StaticMeshBounds.Origin, ForwardAxis) * SplineMeshScaleZ - 0.5f;

		LODResources.Reset(InComponent->StaticMesh->RenderData->LODResources.Num());

		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			FSplineMeshVertexFactory* VertexFactory = new FSplineMeshVertexFactory(this);

			LODResources.Add(VertexFactory);

			InitResources(InComponent, LODIndex);
		}
	}

	virtual ~FSplineMeshSceneProxy() OVERRIDE
	{
		ReleaseResources();

		for (FSplineMeshSceneProxy::FLODResources& LODResource : LODResources)
		{
			delete LODResource.VertexFactory;
		}
		LODResources.Empty();
	}

	void InitResources(USplineMeshComponent* InComponent, int32 InLODIndex);

	void ReleaseResources();

	/** Sets up a shadow FMeshBatch for a specific LOD. */
	virtual bool GetShadowMeshElement(int32 LODIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement) const OVERRIDE
	{
		//checkf(LODIndex == 0, TEXT("Getting spline static mesh element with invalid LOD [%d]"), LODIndex);

		if (FStaticMeshSceneProxy::GetShadowMeshElement(LODIndex, InDepthPriorityGroup, OutMeshElement))
		{
			OutMeshElement.VertexFactory = LODResources[LODIndex].VertexFactory;
			OutMeshElement.ReverseCulling ^= (SplineParams.StartScale.X < 0) ^ (SplineParams.StartScale.Y < 0);
			return true;
		}
		return false;
	}

	/** Sets up a FMeshBatch for a specific LOD and element. */
	virtual bool GetMeshElement(int32 LODIndex, int32 SectionIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement, const bool bUseSelectedMaterial, const bool bUseHoveredMaterial) const OVERRIDE
	{
		//checkf(LODIndex == 0 /*&& SectionIndex == 0*/, TEXT("Getting spline static mesh element with invalid params [%d, %d]"), LODIndex, SectionIndex);

		if (FStaticMeshSceneProxy::GetMeshElement(LODIndex, SectionIndex, InDepthPriorityGroup, OutMeshElement, bUseSelectedMaterial, bUseHoveredMaterial))
		{
			OutMeshElement.VertexFactory = LODResources[LODIndex].VertexFactory;
			OutMeshElement.ReverseCulling ^= (SplineParams.StartScale.X < 0) ^ (SplineParams.StartScale.Y < 0);
			return true;
		}
		return false;
	}

	/** Sets up a wireframe FMeshBatch for a specific LOD. */
	virtual bool GetWireframeMeshElement(int32 LODIndex, const FMaterialRenderProxy* WireframeRenderProxy, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement) const OVERRIDE
	{
		//checkf(LODIndex == 0, TEXT("Getting spline static mesh element with invalid LOD [%d]"), LODIndex);

		if (FStaticMeshSceneProxy::GetWireframeMeshElement(LODIndex, WireframeRenderProxy, InDepthPriorityGroup, OutMeshElement))
		{
			OutMeshElement.VertexFactory = LODResources[LODIndex].VertexFactory;
			OutMeshElement.ReverseCulling ^= (SplineParams.StartScale.X < 0) ^ (SplineParams.StartScale.Y < 0);
			return true;
		}
		return false;
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE
	{
		return FStaticMeshSceneProxy::GetViewRelevance(View);
	}

	// 	  virtual uint32 GetMemoryFootprint( void ) const { return 0; }

	/** Parameters that define the spline, used to deform mesh */
	FSplineMeshParams SplineParams;
	/** Axis (in component space) that is used to determine X axis for co-ordinates along spline */
	FVector SplineUpDir;
	/** Smoothly (cubic) interpolate the Roll and Scale params over spline. */
	bool bSmoothInterpRollScale;
	/** Chooses the forward axis for the spline mesh orientation */
	ESplineMeshAxis::Type ForwardAxis;

	/** Minimum Z value of the entire mesh */
	float SplineMeshMinZ;
	/** Range of Z values over entire mesh */
	float SplineMeshScaleZ;

protected:
	TArray<FLODResources> LODResources;
};