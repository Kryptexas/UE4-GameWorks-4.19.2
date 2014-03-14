// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DrawingPolicy.cpp: Base drawing policy implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"

int32 GEmitMeshDrawEvent = 0;
static FAutoConsoleVariableRef CVarEmitMeshDrawEvent(
	TEXT("r.EmitMeshDrawEvents"),
	GEmitMeshDrawEvent,
	TEXT("Emits a GPU event around each drawing policy draw call.  /n")
	TEXT("Useful for seeing stats about each draw call, however it greatly distorts total time and time per draw call."),
	ECVF_RenderThreadSafe
	);

FMeshDrawingPolicy::FMeshDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FMaterial& InMaterialResource,
	bool bInOverrideWithShaderComplexity,
	bool bInTwoSidedOverride
	):
	VertexFactory(InVertexFactory),
	MaterialRenderProxy(InMaterialRenderProxy),
	MaterialResource(&InMaterialResource),
	bIsTwoSidedMaterial(InMaterialResource.IsTwoSided() || bInTwoSidedOverride),
	bIsWireframeMaterial(InMaterialResource.IsWireframe()),
	bNeedsBackfacePass(
		   !bInTwoSidedOverride
		&& InMaterialResource.IsTwoSided()
		//@todo - hook up bTwoSidedSeparatePass here if we re-add it
		&& false
		),
	//convert from signed bool to unsigned uint32
	bOverrideWithShaderComplexity(bInOverrideWithShaderComplexity != false)
{
}

void FMeshDrawingPolicy::SetMeshRenderState(
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& Mesh,
	int32 BatchElementIndex,
	bool bBackFace,
	const ElementDataType& ElementData
	) const
{
	EmitMeshDrawEvents(PrimitiveSceneProxy, Mesh);

	// Use bitwise logic ops to avoid branches
	RHISetRasterizerState(GetStaticRasterizerState<true>(
		( Mesh.bWireframe | IsWireframe() ) ? FM_Wireframe : FM_Solid, ( ( IsTwoSided() & !NeedsBackfacePass() ) | Mesh.bDisableBackfaceCulling ) ? CM_None :
			( ( (View.bReverseCulling ^ bBackFace) ^ Mesh.ReverseCulling ) ? CM_CCW : CM_CW )
		));
}

void FMeshDrawingPolicy::DrawMesh(const FMeshBatch& Mesh, int32 BatchElementIndex) const
{
	INC_DWORD_STAT(STAT_MeshDrawCalls);
	SCOPED_CONDITIONAL_DRAW_EVENTF(MeshEvent, GEmitMeshDrawEvent != 0, DEC_SCENE_ITEMS, TEXT("Mesh Draw"));

	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	if (Mesh.UseDynamicData)
	{
		check(Mesh.DynamicVertexData);

		if (BatchElement.DynamicIndexData)
		{
			RHIDrawIndexedPrimitiveUP(
				Mesh.Type,
				BatchElement.MinVertexIndex,
				BatchElement.MaxVertexIndex - BatchElement.MinVertexIndex + 1,
				BatchElement.NumPrimitives,
				BatchElement.DynamicIndexData,
				BatchElement.DynamicIndexStride,
				Mesh.DynamicVertexData,
				Mesh.DynamicVertexStride
				);
		}
		else
		{
			RHIDrawPrimitiveUP(
				Mesh.Type,
				BatchElement.NumPrimitives,
				Mesh.DynamicVertexData,
				Mesh.DynamicVertexStride
				);
		}
	}
	else
	{
		if(BatchElement.IndexBuffer)
		{
			check(BatchElement.IndexBuffer->IsInitialized());
			RHIDrawIndexedPrimitive(
				BatchElement.IndexBuffer->IndexBufferRHI,
				Mesh.Type,
				0,
				BatchElement.MinVertexIndex,
				BatchElement.MaxVertexIndex - BatchElement.MinVertexIndex + 1,
				BatchElement.FirstIndex,
				BatchElement.NumPrimitives,
				BatchElement.NumInstances
				);
		}
		else
		{
			RHIDrawPrimitive(
				Mesh.Type,
				BatchElement.FirstIndex,
				BatchElement.NumPrimitives,
				BatchElement.NumInstances
				);
		}
	}
}

void FMeshDrawingPolicy::DrawShared(const FSceneView* View) const
{
	check(VertexFactory && VertexFactory->IsInitialized());
	VertexFactory->Set();
}

/**
* Get the decl and stream strides for this mesh policy type and vertexfactory
* @param VertexDeclaration - output decl 
*/
const FVertexDeclarationRHIRef& FMeshDrawingPolicy::GetVertexDeclaration() const
{
	check(VertexFactory && VertexFactory->IsInitialized());
	const FVertexDeclarationRHIRef& VertexDeclaration = VertexFactory->GetDeclaration();
	check(IsValidRef(VertexDeclaration));
	return VertexDeclaration;
}
