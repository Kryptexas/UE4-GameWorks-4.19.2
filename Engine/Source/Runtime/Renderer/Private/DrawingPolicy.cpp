// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DrawingPolicy.cpp: Base drawing policy implementation.
=============================================================================*/

#include "DrawingPolicy.h"
#include "SceneUtils.h"
#include "SceneRendering.h"
#include "LogMacros.h"
#include "MaterialShader.h"
#include "DebugViewModeRendering.h"

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
	const FMeshDrawingPolicyOverrideSettings& InOverrideSettings,
	EDebugViewShaderMode InDebugViewShaderMode
	):
	VertexFactory(InVertexFactory),
	MaterialRenderProxy(InMaterialRenderProxy),
	MaterialResource(&InMaterialResource),
	MeshPrimitiveType(InOverrideSettings.MeshPrimitiveType),
	bIsDitheredLODTransitionMaterial(InMaterialResource.IsDitheredLODTransition() || !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::DitheredLODTransition)),
	//convert from signed bool to unsigned uint32
	DebugViewShaderMode((uint32)InDebugViewShaderMode)
{
	// using this saves a virtual function call
	bool bMaterialResourceIsTwoSided = InMaterialResource.IsTwoSided();
	
	const bool bIsWireframeMaterial = InMaterialResource.IsWireframe() || !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::Wireframe);
	MeshFillMode = bIsWireframeMaterial ? FM_Wireframe : FM_Solid;

	const bool bInTwoSidedOverride = !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::TwoSided);
	const bool bInReverseCullModeOverride = !!(InOverrideSettings.MeshOverrideFlags & EDrawingPolicyOverrideFlags::ReverseCullMode);
	const bool bIsTwoSided = (bMaterialResourceIsTwoSided || bInTwoSidedOverride);
	const bool bMeshRenderTwoSided = bIsTwoSided || bInTwoSidedOverride;
	MeshCullMode = (bMeshRenderTwoSided) ? CM_None : (bInReverseCullModeOverride ? CM_CCW : CM_CW);

	bUsePositionOnlyVS = false;
}

void FMeshDrawingPolicy::OnlyApplyDitheredLODTransitionState(FDrawingPolicyRenderState& DrawRenderState, const FViewInfo& ViewInfo, const FStaticMesh& Mesh, const bool InAllowStencilDither)
{
	DrawRenderState.SetDitheredLODTransitionAlpha(0.0f);

	if (Mesh.bDitheredLODTransition && !InAllowStencilDither)
	{
		if (ViewInfo.StaticMeshFadeOutDitheredLODMap[Mesh.Id])
		{
			DrawRenderState.SetDitheredLODTransitionAlpha(ViewInfo.GetTemporalLODTransition());
		}
		else if (ViewInfo.StaticMeshFadeInDitheredLODMap[Mesh.Id])
		{
			DrawRenderState.SetDitheredLODTransitionAlpha(ViewInfo.GetTemporalLODTransition() - 1.0f);
		}
	}
}

void FMeshDrawingPolicy::SetInstanceParameters(FRHICommandList& RHICmdList, const FSceneView& View, uint32 InVertexOffset, uint32 InInstanceOffset, uint32 InInstanceCount) const
{
	if (UseDebugViewPS() && View.Family->UseDebugViewVSDSHS())
	{
		FDebugViewMode::SetInstanceParameters(RHICmdList, VertexFactory, View, MaterialResource, InVertexOffset, InInstanceOffset, InInstanceCount);
	}
	else
	{
		BaseVertexShader->SetInstanceParameters(RHICmdList, InVertexOffset, InInstanceOffset, InInstanceCount);
	}
}

void FMeshDrawingPolicy::DrawMesh(FRHICommandList& RHICmdList, const FSceneView& View, const FMeshBatch& Mesh, int32 BatchElementIndex, const bool bIsInstancedStereo) const
{
	DEFINE_LOG_CATEGORY_STATIC(LogFMeshDrawingPolicyDrawMesh, Warning, All);
	INC_DWORD_STAT(STAT_MeshDrawCalls);
	SCOPED_CONDITIONAL_DRAW_EVENTF(RHICmdList, MeshEvent, GEmitMeshDrawEvent != 0, TEXT("Mesh Draw"));

	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	if (BatchElement.IndexBuffer)
	{
		UE_CLOG(!BatchElement.IndexBuffer->IndexBufferRHI, LogFMeshDrawingPolicyDrawMesh, Fatal,
			TEXT("FMeshDrawingPolicy::DrawMesh - BatchElement has an index buffer object with null RHI resource (drawing using material \"%s\")"),
			MaterialRenderProxy ? *MaterialRenderProxy->GetFriendlyName() : TEXT("null"));
		check(BatchElement.IndexBuffer->IsInitialized());

		if (BatchElement.bIsInstanceRuns)
		{
			checkSlow(BatchElement.bIsInstanceRuns);
			if (!GRHISupportsFirstInstance)
			{
				if (GetUsePositionOnlyVS())
				{
					for (uint32 Run = 0; Run < BatchElement.NumInstances; Run++)
					{
						const uint32 InstanceCount = (1 + BatchElement.InstanceRuns[Run * 2 + 1] - BatchElement.InstanceRuns[Run * 2]);
						SetInstanceParameters(RHICmdList, View, BatchElement.BaseVertexIndex, 0, InstanceCount);
						GetVertexFactory()->OffsetPositionInstanceStreams(RHICmdList, BatchElement.InstanceRuns[Run * 2]);

						RHICmdList.DrawIndexedPrimitive(
							BatchElement.IndexBuffer->IndexBufferRHI,
							Mesh.Type,
							BatchElement.BaseVertexIndex,
							0,
							BatchElement.MaxVertexIndex - BatchElement.MinVertexIndex + 1,
							BatchElement.FirstIndex,
							BatchElement.NumPrimitives,
							InstanceCount * GetInstanceFactor()
						);
					}
				}
				else
				{
					for (uint32 Run = 0; Run < BatchElement.NumInstances; Run++)
					{
						uint32 InstanceCount = (1 + BatchElement.InstanceRuns[Run * 2 + 1] - BatchElement.InstanceRuns[Run * 2]);
						SetInstanceParameters(RHICmdList, View, BatchElement.BaseVertexIndex, 0, InstanceCount);
						GetVertexFactory()->OffsetInstanceStreams(RHICmdList, BatchElement.InstanceRuns[Run * 2]);

						RHICmdList.DrawIndexedPrimitive(
							BatchElement.IndexBuffer->IndexBufferRHI,
							Mesh.Type,
							BatchElement.BaseVertexIndex,
							0,
							BatchElement.MaxVertexIndex - BatchElement.MinVertexIndex + 1,
							BatchElement.FirstIndex,
							BatchElement.NumPrimitives,
							InstanceCount * GetInstanceFactor()
						);
					}
				}
			}
			else
			{
				for (uint32 Run = 0; Run < BatchElement.NumInstances; Run++)
				{
					const uint32 InstanceOffset = BatchElement.InstanceRuns[Run * 2];
					const uint32 InstanceCount = (1 + BatchElement.InstanceRuns[Run * 2 + 1] - BatchElement.InstanceRuns[Run * 2]);
					SetInstanceParameters(RHICmdList, View, BatchElement.BaseVertexIndex, InstanceOffset, InstanceCount);

					RHICmdList.DrawIndexedPrimitive(
						BatchElement.IndexBuffer->IndexBufferRHI,
						Mesh.Type,
						BatchElement.BaseVertexIndex,
						InstanceOffset,
						BatchElement.MaxVertexIndex - BatchElement.MinVertexIndex + 1,
						BatchElement.FirstIndex,
						BatchElement.NumPrimitives,
						InstanceCount * GetInstanceFactor()
					);
				}
			}
		}
		else
		{
			if (BatchElement.IndirectArgsBuffer)
			{
				RHICmdList.DrawIndexedPrimitiveIndirect(
					Mesh.Type, 
					BatchElement.IndexBuffer->IndexBufferRHI, 
					BatchElement.IndirectArgsBuffer, 
					0
				);
			}
			else
			{
				// Currently only supporting this path for instanced stereo.
				const uint32 InstanceCount = ((bIsInstancedStereo && !BatchElement.bIsInstancedMesh) ? 2 : BatchElement.NumInstances);
				SetInstanceParameters(RHICmdList, View, BatchElement.BaseVertexIndex, 0, InstanceCount);

				RHICmdList.DrawIndexedPrimitive(
					BatchElement.IndexBuffer->IndexBufferRHI,
					Mesh.Type,
					BatchElement.BaseVertexIndex,
					0,
					BatchElement.MaxVertexIndex - BatchElement.MinVertexIndex + 1,
					BatchElement.FirstIndex,
					BatchElement.NumPrimitives,
					InstanceCount * GetInstanceFactor()
				);
			}
		}
	}
	else
	{
		SetInstanceParameters(RHICmdList, View, BatchElement.BaseVertexIndex + BatchElement.FirstIndex, 0, 1);

		RHICmdList.DrawPrimitive(
			Mesh.Type,
			BatchElement.BaseVertexIndex + BatchElement.FirstIndex,
			BatchElement.NumPrimitives,
			BatchElement.NumInstances
		);
	}
}

void FMeshDrawingPolicy::SetSharedState(FRHICommandList& RHICmdList, const FDrawingPolicyRenderState& DrawRenderState, const FSceneView* View, const FMeshDrawingPolicy::ContextDataType PolicyContext) const
{
	check(VertexFactory && VertexFactory->IsInitialized());
	VertexFactory->Set(View->GetShaderPlatform(), RHICmdList);
}

/**
* Get the decl and stream strides for this mesh policy type and vertexfactory
* @param VertexDeclaration - output decl 
*/
const FVertexDeclarationRHIRef& FMeshDrawingPolicy::GetVertexDeclaration() const
{
	check(VertexFactory && VertexFactory->IsInitialized());
	const FVertexDeclarationRHIRef& VertexDeclaration = VertexFactory->GetDeclaration();
	check(VertexFactory->NeedsDeclaration()==false || IsValidRef(VertexDeclaration));

	return VertexDeclaration;
}
