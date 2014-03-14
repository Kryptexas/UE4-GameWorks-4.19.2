// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticMeshRender.cpp: Static mesh rendering code.
=============================================================================*/

#include "EnginePrivate.h"
#include "LevelUtils.h"
#include "TessellationRendering.h"

/** If true, optimized depth-only index buffers are used for shadow rendering. */
static bool GUseShadowIndexBuffer = true;

static void ToggleShadowIndexBuffers()
{
	FlushRenderingCommands();
	GUseShadowIndexBuffer = !GUseShadowIndexBuffer;
	UE_LOG(LogStaticMesh,Log,TEXT("Optimized shadow index buffers %s"),GUseShadowIndexBuffer ? TEXT("ENABLED") : TEXT("DISABLED"));
	FGlobalComponentReregisterContext ReregisterContext;
}

static FAutoConsoleCommand GToggleShadowIndexBuffersCmd(
	TEXT("ToggleShadowIndexBuffers"),
	TEXT("Render static meshes with an optimized shadow index buffer that minimizes unique vertices."),
	FConsoleCommandDelegate::CreateStatic(ToggleShadowIndexBuffers)
	);

bool GForceDefaultMaterial = false;

static void ToggleForceDefaultMaterial()
{
	FlushRenderingCommands();
	GForceDefaultMaterial = !GForceDefaultMaterial;
	UE_LOG(LogStaticMesh,Log,TEXT("Force default material %s"),GUseShadowIndexBuffer ? TEXT("ENABLED") : TEXT("DISABLED"));
	FGlobalComponentReregisterContext ReregisterContext;
}

static FAutoConsoleCommand GToggleForceDefaultMaterialCmd(
	TEXT("ToggleForceDefaultMaterial"),
	TEXT("Render all meshes with the default material."),
	FConsoleCommandDelegate::CreateStatic(ToggleForceDefaultMaterial)
	);

/** Initialization constructor. */
FStaticMeshSceneProxy::FStaticMeshSceneProxy(UStaticMeshComponent* InComponent):
	FPrimitiveSceneProxy(InComponent, InComponent->StaticMesh->GetFName()),
	Owner(InComponent->GetOwner()),
	StaticMesh(InComponent->StaticMesh),
	BodySetup(InComponent->GetBodySetup()),
	RenderData(InComponent->StaticMesh->RenderData),
	ForcedLodModel(InComponent->ForcedLodModel),
	LevelColor(1,1,1),
	PropertyColor(1,1,1),
	bCastShadow(InComponent->CastShadow),
	CollisionTraceFlag(ECollisionTraceFlag::CTF_UseDefault),
	MaterialRelevance(InComponent->GetMaterialRelevance()),
	WireframeColor(InComponent->GetWireframeColor()), 
	CollisionResponse(InComponent->GetCollisionResponseToChannels())
{
	check(RenderData);

	if (GForceDefaultMaterial)
	{
		MaterialRelevance |= UMaterial::GetDefaultMaterial(MD_Surface)->GetRelevance();
	}

	// Build the proxy's LOD data.
	bool bAnySectionCastsShadows = false;
	LODs.Empty(RenderData->LODResources.Num());
	for(int32 LODIndex = 0;LODIndex < RenderData->LODResources.Num();LODIndex++)
	{
		FLODInfo* NewLODInfo = new(LODs) FLODInfo(InComponent,LODIndex);

		// Under certain error conditions an LOD's material will be set to 
		// DefaultMaterial. Ensure our material view relevance is set properly.
		const int32 NumSections = NewLODInfo->Sections.Num();
		for (int32 SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex)
		{
			const FLODInfo::FSectionInfo& SectionInfo = NewLODInfo->Sections[SectionIndex];
			bAnySectionCastsShadows |= RenderData->LODResources[LODIndex].Sections[SectionIndex].bCastShadow;
			if (SectionInfo.Material == UMaterial::GetDefaultMaterial(MD_Surface))
			{
				MaterialRelevance |= UMaterial::GetDefaultMaterial(MD_Surface)->GetRelevance();
			}
		}
	}

	// Disable shadow casting if no section has it enabled.
	bCastShadow = bCastShadow && bAnySectionCastsShadows;
	bCastDynamicShadow = bCastDynamicShadow && bCastShadow;

	bStaticElementsAlwaysUseProxyPrimitiveUniformBuffer = true;

	LpvBiasMultiplier = FMath::Min( InComponent->StaticMesh->LpvBiasMultiplier * InComponent->LpvBiasMultiplier, 3.0f );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR
	if( GIsEditor )
	{
		// Try to find a color for level coloration.
		if ( Owner )
		{
			ULevel* Level = Owner->GetLevel();
			ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
			if ( LevelStreaming )
			{
				LevelColor = LevelStreaming->DrawColor;
			}
		}

		// Get a color for property coloration.
		FColor TempPropertyColor;
		if (GEngine->GetPropertyColorationColor( (UObject*)InComponent, TempPropertyColor ))
		{
			PropertyColor = TempPropertyColor;
		}
	}
#endif

	if (BodySetup)
	{
		CollisionTraceFlag = BodySetup->CollisionTraceFlag;
	}
}

void UStaticMeshComponent::SetLODDataCount( const uint32 MinSize, const uint32 MaxSize )
{
	check(MaxSize <= MAX_STATIC_MESH_LODS);
	if (MaxSize < (uint32)LODData.Num())
	{
		// call destructors
		LODData.RemoveAt(MaxSize, LODData.Num() - MaxSize);
	}
	
	if(MinSize > (uint32)LODData.Num())
	{
		// call constructors
		LODData.Reserve(MinSize);

		// TArray doesn't have a function for constructing n items
		uint32 ItemCountToAdd = MinSize - LODData.Num();
		for(uint32 i = 0; i < ItemCountToAdd; ++i)
		{
			// call constructor
			new (LODData)FStaticMeshComponentLODInfo();
		}
	}
}

bool FStaticMeshSceneProxy::GetShadowMeshElement(int32 LODIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement) const
{
	const FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];
	const FLODInfo& ProxyLODInfo = LODs[LODIndex];

	FMeshBatchElement& OutBatchElement = OutMeshElement.Elements[0];
	OutMeshElement.MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false, false);
	OutMeshElement.VertexFactory = &LOD.VertexFactory;
	OutBatchElement.IndexBuffer = &LOD.DepthOnlyIndexBuffer;
	OutMeshElement.Type = PT_TriangleList;
	OutBatchElement.FirstIndex = 0;
	OutBatchElement.NumPrimitives = LOD.DepthOnlyIndexBuffer.GetNumIndices() / 3;
	OutBatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
	OutBatchElement.MinVertexIndex = 0;
	OutBatchElement.MaxVertexIndex = LOD.PositionVertexBuffer.GetNumVertices() - 1;
	OutMeshElement.DepthPriorityGroup = InDepthPriorityGroup;
	OutMeshElement.ReverseCulling = IsLocalToWorldDeterminantNegative();
	OutMeshElement.CastShadow = true;
	OutMeshElement.LODIndex = LODIndex;
	OutMeshElement.LCI = &ProxyLODInfo;

	return true;
}

/** Sets up a FMeshBatch for a specific LOD and element. */
bool FStaticMeshSceneProxy::GetMeshElement(int32 LODIndex,int32 SectionIndex,uint8 InDepthPriorityGroup,FMeshBatch& OutMeshElement, const bool bUseSelectedMaterial, const bool bUseHoveredMaterial) const
{
	const FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];
	const FStaticMeshSection& Section = LOD.Sections[SectionIndex];

	const FLODInfo& ProxyLODInfo = LODs[LODIndex];
	UMaterialInterface* Material = ProxyLODInfo.Sections[SectionIndex].Material;
	OutMeshElement.MaterialRenderProxy = Material->GetRenderProxy(bUseSelectedMaterial,bUseHoveredMaterial);
	OutMeshElement.VertexFactory = &LOD.VertexFactory;

	// Has the mesh component overridden the vertex color stream for this mesh LOD?
	if( ProxyLODInfo.OverrideColorVertexBuffer != NULL )
	{
		check( ProxyLODInfo.OverrideColorVertexFactory != NULL );

		// Make sure the indices are accessing data within the vertex buffer's
		if(Section.MaxVertexIndex < ProxyLODInfo.OverrideColorVertexBuffer->GetNumVertices())
		{
			// Switch out the stock mesh vertex factory with own own vertex factory that points to
			// our overridden color data
			OutMeshElement.VertexFactory = ProxyLODInfo.OverrideColorVertexFactory.GetOwnedPointer();
		}
	}

	const bool bWireframe = false;
	const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation( Material, OutMeshElement.VertexFactory->GetType() );

	SetIndexSource(LODIndex, SectionIndex, OutMeshElement, bWireframe, bRequiresAdjacencyInformation );

	FMeshBatchElement& OutBatchElement = OutMeshElement.Elements[0];

	if(OutBatchElement.NumPrimitives > 0)
	{
		OutMeshElement.DynamicVertexData = NULL;
		OutMeshElement.LCI = &ProxyLODInfo;
		OutBatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
		OutBatchElement.MinVertexIndex = Section.MinVertexIndex;
		OutBatchElement.MaxVertexIndex = Section.MaxVertexIndex;
		OutMeshElement.LODIndex = LODIndex;
		OutMeshElement.UseDynamicData = false;
		OutMeshElement.ReverseCulling = IsLocalToWorldDeterminantNegative();
		OutMeshElement.CastShadow = bCastShadow && Section.bCastShadow;
		OutMeshElement.DepthPriorityGroup = (ESceneDepthPriorityGroup)InDepthPriorityGroup;
		return true;
	}
	else
	{
		return false;
	}
}

/** Sets up a wireframe FMeshBatch for a specific LOD. */
bool FStaticMeshSceneProxy::GetWireframeMeshElement(int32 LODIndex, const FMaterialRenderProxy* WireframeRenderProxy, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement) const
{
	const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
	
	FMeshBatchElement& OutBatchElement = OutMeshElement.Elements[0];

	OutMeshElement.VertexFactory = &LODModel.VertexFactory;
	OutMeshElement.MaterialRenderProxy = WireframeRenderProxy;
	OutBatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
	OutBatchElement.MinVertexIndex = 0;
	OutBatchElement.MaxVertexIndex = LODModel.GetNumVertices() - 1;
	OutMeshElement.ReverseCulling = IsLocalToWorldDeterminantNegative();
	OutMeshElement.CastShadow = bCastShadow;
	OutMeshElement.DepthPriorityGroup = (ESceneDepthPriorityGroup)InDepthPriorityGroup;

	const bool bWireframe = true;
	const bool bRequiresAdjacencyInformation = false;

	SetIndexSource(LODIndex, 0, OutMeshElement, bWireframe, bRequiresAdjacencyInformation);

	return OutBatchElement.NumPrimitives > 0;
}

/**
 * Sets IndexBuffer, FirstIndex and NumPrimitives of OutMeshElement.
 */
void FStaticMeshSceneProxy::SetIndexSource(int32 LODIndex, int32 SectionIndex, FMeshBatch& OutMeshElement, bool bWireframe, bool bRequiresAdjacencyInformation) const
{
	FMeshBatchElement& OutElement = OutMeshElement.Elements[0];
	const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
	if (bWireframe)
	{
		if( LODModel.WireframeIndexBuffer.IsInitialized()
			&& !(RHISupportsTessellation(GRHIShaderPlatform) && OutMeshElement.VertexFactory->GetType()->SupportsTessellationShaders())
			)
		{
			OutMeshElement.Type = PT_LineList;
			OutElement.FirstIndex = 0;
			OutElement.IndexBuffer = &LODModel.WireframeIndexBuffer;
			OutElement.NumPrimitives = LODModel.WireframeIndexBuffer.GetNumIndices() / 2;
		}
		else
		{
			OutMeshElement.Type = PT_TriangleList;
			OutElement.FirstIndex = 0;
			OutElement.IndexBuffer = &LODModel.IndexBuffer;
			OutElement.NumPrimitives = LODModel.IndexBuffer.GetNumIndices() / 3;
			OutMeshElement.bWireframe = true;
			OutMeshElement.bDisableBackfaceCulling = true;
		}
	}
	else
	{
		const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
		OutMeshElement.Type = PT_TriangleList;
		OutElement.IndexBuffer = &LODModel.IndexBuffer;
		OutElement.FirstIndex = Section.FirstIndex;
		OutElement.NumPrimitives = Section.NumTriangles;
	}

	if ( bRequiresAdjacencyInformation )
	{
		check( LODModel.bHasAdjacencyInfo );
		OutElement.IndexBuffer = &LODModel.AdjacencyIndexBuffer;
		OutMeshElement.Type = PT_12_ControlPointPatchList;
		OutElement.FirstIndex *= 4;
	}
}

// FPrimitiveSceneProxy interface.
void FStaticMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
{
	checkSlow(IsInRenderingThread());
	if (!HasViewDependentDPG())
	{
		// Determine the DPG the primitive should be drawn in.
		uint8 PrimitiveDPG = GetStaticDepthPriorityGroup();
		int32 NumLODs = RenderData->LODResources.Num();
		//Never use the dynamic path in this path, because only unselected elements will use DrawStaticElements
		bool bUseSelectedMaterial = false;
		const bool bUseHoveredMaterial = false;
		

		//check if a LOD is being forced
		if (ForcedLodModel > 0) 
		{
			int32 LODIndex = FMath::Clamp(ForcedLodModel, 1, NumLODs) - 1;
			const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
			// Draw the static mesh elements.
			for(int32 SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
			{
#if WITH_EDITOR
				if( GIsEditor )
				{
					bUseSelectedMaterial = LODs[LODIndex].Sections[SectionIndex].bSelected;
				}
#endif // WITH_EDITOR
				FMeshBatch MeshElement;
				if(GetMeshElement(LODIndex,SectionIndex,PrimitiveDPG,MeshElement,bUseSelectedMaterial,bUseHoveredMaterial))
				{
					PDI->DrawMesh(MeshElement, 0, FLT_MAX);
				}
			}
		} 
		else //no LOD is being forced, submit them all with appropriate cull distances
		{
			for(int32 LODIndex = 0;LODIndex < NumLODs;LODIndex++)
			{
				const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
				float MinDist = GetMinLODDist(LODIndex);
				float MaxDist = GetMaxLODDist(LODIndex);

				bool bHaveShadowOnlyMesh = false;
				if (GUseShadowIndexBuffer
					&& bCastShadow
					&& LODModel.DepthOnlyIndexBuffer.GetNumIndices() > 0)
				{
					const FLODInfo& ProxyLODInfo = LODs[LODIndex];

					// The shadow-only mesh can be used only if all elements cast shadows and use opaque materials with no vertex modification.
					bool bSafeToUseShadowOnlyMesh = true;
					bool bAnySectionCastsShadow = false;
					for (int32 SectionIndex = 0; bSafeToUseShadowOnlyMesh && SectionIndex < LODModel.Sections.Num(); SectionIndex++)
					{
						const FMaterial* Material = ProxyLODInfo.Sections[SectionIndex].Material->GetRenderProxy(false)->GetMaterial(GRHIFeatureLevel);
						const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
						bSafeToUseShadowOnlyMesh =
							Section.bCastShadow
							&& Material->GetBlendMode() == BLEND_Opaque
							&& !Material->IsTwoSided()
							&& !Material->MaterialModifiesMeshPosition();
						bAnySectionCastsShadow |= Section.bCastShadow;
					}

					bSafeToUseShadowOnlyMesh = false;

					if (bAnySectionCastsShadow && bSafeToUseShadowOnlyMesh)
					{
						FMeshBatch MeshElement;
						if (GetShadowMeshElement(LODIndex, PrimitiveDPG, MeshElement))
						{
							bHaveShadowOnlyMesh = true;
							PDI->DrawMesh(MeshElement,MinDist,MaxDist,/*bShadowOnly=*/true);
						}
					}
				}

				// Draw the static mesh elements.
				for(int32 SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
				{
#if WITH_EDITOR
					if( GIsEditor )
					{
						bUseSelectedMaterial = LODs[LODIndex].Sections[SectionIndex].bSelected;
					}
#endif // WITH_EDITOR
					FMeshBatch MeshElement;
					if(GetMeshElement(LODIndex,SectionIndex,PrimitiveDPG,MeshElement,bUseSelectedMaterial,bUseHoveredMaterial))
					{
						// If we have submitted an optimized shadow-only mesh, remaining mesh elements must not cast shadows.
						MeshElement.CastShadow = MeshElement.CastShadow && !bHaveShadowOnlyMesh;
						PDI->DrawMesh(MeshElement, MinDist, MaxDist);
					}
				}
			}
		}
	}
}

bool FStaticMeshSceneProxy::IsCollisionView(const FSceneView* View, bool & bDrawSimpleCollision, bool & bDrawComplexCollision) const
{
	bDrawSimpleCollision = bDrawComplexCollision = false;

	const bool bInCollisionView = View->Family->EngineShowFlags.CollisionVisibility || View->Family->EngineShowFlags.CollisionPawn;
	// If in a 'collision view' and collision is enabled
	if (bInCollisionView && IsCollisionEnabled())
	{
		// See if we have a response to the interested channel
		bool bHasResponse = View->Family->EngineShowFlags.CollisionPawn && CollisionResponse.GetResponse(ECC_Pawn) != ECR_Ignore;
		bHasResponse |= View->Family->EngineShowFlags.CollisionVisibility && CollisionResponse.GetResponse(ECC_Visibility) != ECR_Ignore;

		if(bHasResponse)
		{
			bDrawComplexCollision = View->Family->EngineShowFlags.CollisionVisibility;
			bDrawSimpleCollision = View->Family->EngineShowFlags.CollisionPawn;
		}
	}

	return bInCollisionView;
}
/** 
* Draw the scene proxy as a dynamic element
*
* @param	PDI - draw interface to render to
* @param	View - current view
*/
void FStaticMeshSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View, uint32 DrawDynamicFlags )
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_StaticMeshSceneProxy_DrawDynamicElements );

	checkSlow(IsInRenderingThread());

	const bool bIsLightmapSettingError = HasStaticLighting() && !HasValidSettingsForStaticLighting();

	bool bProxyIsSelected = IsSelected();

	bool bDrawSimpleCollision = false, bDrawComplexCollision = false;
	const bool bInCollisionView = IsCollisionView(View, bDrawSimpleCollision, bDrawComplexCollision);
	// Draw simple collision as wireframe if 'show collision', collision is enabled, and we are not using the complex as the simple
	const bool bDrawWireframeCollision = (View->Family->EngineShowFlags.Collision && IsCollisionEnabled() && CollisionTraceFlag != ECollisionTraceFlag::CTF_UseComplexAsSimple);

	const bool bDrawMesh = (bInCollisionView) ? (bDrawComplexCollision) : 
		(	IsRichView(View) || HasViewDependentDPG()
			|| View->Family->EngineShowFlags.Collision
			|| View->Family->EngineShowFlags.Bounds
			|| bProxyIsSelected 
			|| IsHovered()
			|| bIsLightmapSettingError ) ;


	// Draw polygon mesh if we are either not in a collision view, or are drawing it as collision.
	if(View->Family->EngineShowFlags.StaticMeshes && bDrawMesh)
	{
		// how we should draw the collision for this mesh.
		const bool bIsWireframeView = View->Family->EngineShowFlags.Wireframe;
		const bool bLevelColorationEnabled = View->Family->EngineShowFlags.LevelColoration;
		const bool bPropertyColorationEnabled = View->Family->EngineShowFlags.PropertyColoration;

		// force lowest LOD (e.g. for Reflective Shadow Maps)
		int32 LODIndex = 0;
		if(DrawDynamicFlags & EDrawDynamicFlags::ForceLowestLOD)
		{
			LODIndex = RenderData->LODResources.Num() - 1;
		} 
		else
		{
			LODIndex = GetLOD(View);
		}

		if (RenderData->LODResources.IsValidIndex(LODIndex))
		{
			const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
			const FLODInfo& ProxyLODInfo = LODs[LODIndex];

			if (AllowDebugViewmodes() && bIsWireframeView && !View->Family->EngineShowFlags.Materials
				// If any of the materials are mesh-modifying, we can't use the single merged mesh element of GetWireframeMeshElement()
				&& !ProxyLODInfo.UsesMeshModifyingMaterials())
			{
				FLinearColor ViewWireframeColor( bLevelColorationEnabled ? LevelColor : WireframeColor );
				if ( bPropertyColorationEnabled )
				{
					ViewWireframeColor = PropertyColor;
				}

				FColoredMaterialRenderProxy WireframeMaterialInstance(
					GEngine->WireframeMaterial->GetRenderProxy(false),
					GetSelectionColor(ViewWireframeColor,!(GIsEditor && View->Family->EngineShowFlags.Selection) || bProxyIsSelected, IsHovered(), /*bUseOverlayIntensity=*/false)
					);

				FMeshBatch Mesh;

				if (GetWireframeMeshElement(LODIndex, &WireframeMaterialInstance, GetDepthPriorityGroup(View), Mesh))
				{
					const int32 NumPasses = PDI->DrawMesh(Mesh);

					INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,Mesh.GetNumPrimitives() * NumPasses);
				}
			}
			else
			{
				const FLinearColor UtilColor( LevelColor );

				// Draw the static mesh sections.
				for(int32 SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
				{
					bool bSectionIsSelected = bProxyIsSelected;

#if WITH_EDITOR
					if( GIsEditor )
					{
						bSectionIsSelected = bSectionIsSelected || LODs[LODIndex].Sections[SectionIndex].bSelected;
					}
#endif // WITH_EDITOR
					FMeshBatch MeshElement;
					if(GetMeshElement(LODIndex,SectionIndex,GetDepthPriorityGroup(View),MeshElement,bSectionIsSelected,IsHovered()))
					{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
						if( View->Family->EngineShowFlags.VertexColors && AllowDebugViewmodes() )
						{
							// Override the mesh's material with our material that draws the vertex colors
							UMaterial* VertexColorVisualizationMaterial = NULL;
							switch( GVertexColorViewMode )
							{
							case EVertexColorViewMode::Color:
								VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_ColorOnly;
								break;

							case EVertexColorViewMode::Alpha:
								VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_AlphaAsColor;
								break;

							case EVertexColorViewMode::Red:
								VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_RedOnly;
								break;

							case EVertexColorViewMode::Green:
								VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_GreenOnly;
								break;

							case EVertexColorViewMode::Blue:
								VertexColorVisualizationMaterial = GEngine->VertexColorViewModeMaterial_BlueOnly;
								break;
							}
							check( VertexColorVisualizationMaterial != NULL );

							const FColoredMaterialRenderProxy VertexColorVisualizationMaterialInstance(
								VertexColorVisualizationMaterial->GetRenderProxy( MeshElement.MaterialRenderProxy->IsSelected(),MeshElement.MaterialRenderProxy->IsHovered() ),
								GetSelectionColor( FLinearColor::White, bSectionIsSelected, IsHovered() )
								);
							FMeshBatch ModifiedMeshElement = MeshElement;
							ModifiedMeshElement.MaterialRenderProxy = &VertexColorVisualizationMaterialInstance;

							const int32 NumPasses = DrawRichMesh(
								PDI,
								ModifiedMeshElement,
								WireframeColor,
								UtilColor,
								PropertyColor,
								this,
								bSectionIsSelected,
								bIsWireframeView
								);
							INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,MeshElement.GetNumPrimitives() * NumPasses);
						}
						else if( bInCollisionView && bDrawComplexCollision && AllowDebugViewmodes() )
						{
							if (LODModel.Sections[SectionIndex].bEnableCollision)
							{
								// Override the mesh's material with our material that draws the collision color
								const FColoredMaterialRenderProxy CollisionMaterialInstance(
									GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(bProxyIsSelected, IsHovered()),
									WireframeColor
									);

								FMeshBatch ModifiedMeshElement = MeshElement;
								ModifiedMeshElement.MaterialRenderProxy = &CollisionMaterialInstance;

								const int32 NumPasses = DrawRichMesh(
									PDI,
									ModifiedMeshElement,
									WireframeColor,
									UtilColor,
									PropertyColor,
									this,
									bSectionIsSelected,
									bIsWireframeView
									);
								INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,MeshElement.GetNumPrimitives() * NumPasses);
							}
						}
						else
#endif
						{
							const int32 NumPasses = DrawRichMesh(
								PDI,
								MeshElement,
								WireframeColor,
								UtilColor,
								PropertyColor,
								this,
								bSectionIsSelected,
								bIsWireframeView
								);
							INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,MeshElement.GetNumPrimitives() * NumPasses);
						}

					}
				}
			}
		}
	}


	if((bDrawSimpleCollision || bDrawWireframeCollision) && AllowDebugViewmodes())
	{
		if(BodySetup)
		{
			if(FMath::Abs(GetLocalToWorld().Determinant()) < SMALL_NUMBER)
			{
				// Catch this here or otherwise GeomTransform below will assert
				// This spams so commented out
				//UE_LOG(LogStaticMesh, Log, TEXT("Zero scaling not supported (%s)"), *StaticMesh->GetPathName());
			}
			else
			{
				const bool bDrawSolid = !bDrawWireframeCollision;

				if(bDrawSolid)
				{
					// Make a material for drawing solid collision stuff
					const FColoredMaterialRenderProxy SolidMaterialInstance(
						GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(IsSelected(), IsHovered()),
						WireframeColor
						);

					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, WireframeColor, /*Material=*/&SolidMaterialInstance, false, /*bSolid=*/true);
				}
				// wireframe
				else
				{
					FColor CollisionColor = FColor(157,149,223,255);
					FTransform GeomTransform(GetLocalToWorld());
					BodySetup->AggGeom.DrawAggGeom(PDI, GeomTransform, GetSelectionColor(CollisionColor, bProxyIsSelected, IsHovered()), /*Material=*/NULL, (Owner == NULL), /*bSolid=*/false);
				}


				// The simple nav geometry is only used by dynamic obstacles for now
				if (StaticMesh->NavCollision && StaticMesh->NavCollision->bIsDynamicObstacle)
				{
					// Draw the static mesh's body setup (simple collision)
					FTransform GeomTransform(GetLocalToWorld());
					FColor NavCollisionColor = FColor(118,84,255,255);
					StaticMesh->NavCollision->DrawSimpleGeom(PDI, GeomTransform, GetSelectionColor(NavCollisionColor, bProxyIsSelected, IsHovered()));
				}
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (View->Family->EngineShowFlags.StaticMeshes)
	{
		RenderBounds(PDI, View->Family->EngineShowFlags, GetBounds(), !Owner || IsSelected());
	}
#endif
}

void FStaticMeshSceneProxy::OnTransformChanged()
{
	// Update the cached scaling.
	const FMatrix& LocalToWorld = GetLocalToWorld();
	TotalScale3D.X = FVector(LocalToWorld.TransformVector(FVector(1,0,0))).Size();
	TotalScale3D.Y = FVector(LocalToWorld.TransformVector(FVector(0,1,0))).Size();
	TotalScale3D.Z = FVector(LocalToWorld.TransformVector(FVector(0,0,1))).Size();
}

bool FStaticMeshSceneProxy::CanBeOccluded() const
{
	return !MaterialRelevance.bDisableDepthTest;
}

FPrimitiveViewRelevance FStaticMeshSceneProxy::GetViewRelevance(const FSceneView* View)
{   
	checkSlow(IsInRenderingThread());

	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.StaticMeshes;
	Result.bRenderCustomDepth = ShouldRenderCustomDepth();
	Result.bRenderInMainPass = ShouldRenderInMainPass();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR
	bool bDrawSimpleCollision = false, bDrawComplexCollision = false;
	const bool bInCollisionView = IsCollisionView(View, bDrawSimpleCollision, bDrawComplexCollision);
#endif

	if(
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR
		IsRichView(View) || 
		View->Family->EngineShowFlags.Collision ||
		bInCollisionView ||
		View->Family->EngineShowFlags.Bounds ||
#endif
		// Force down dynamic rendering path if invalid lightmap settings, so we can apply an error material in DrawRichMesh
		(HasStaticLighting() && !HasValidSettingsForStaticLighting()) ||
		HasViewDependentDPG() 
#if WITH_EDITOR
		//only check these in the editor
		|| IsSelected() || IsHovered()
#endif
		)
	{
		Result.bDynamicRelevance = true;
	}
	else
	{
		Result.bStaticRelevance = true;
	}

	Result.bShadowRelevance = IsShadowCast(View);

	MaterialRelevance.SetPrimitiveViewRelevance(Result);

	if (!View->Family->EngineShowFlags.Materials 
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR
		|| bInCollisionView
#endif
		)
	{
		Result.bOpaqueRelevance = true;
	}
	return Result;
}

void FStaticMeshSceneProxy::GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const
{
	// Attach the light to the primitive's static meshes.
	bDynamic = true;
	bRelevant = false;
	bLightMapped = true;
	bShadowMapped = true;

	if (LODs.Num() > 0)
	{
		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			const FLODInfo* LCI = &LODs[LODIndex];

			if (LCI)
			{
				ELightInteractionType InteractionType = LCI->GetInteraction(LightSceneProxy).GetType();

				if (InteractionType != LIT_CachedIrrelevant)
				{
					bRelevant = true;
				}

				if (InteractionType != LIT_CachedLightMap && InteractionType != LIT_CachedIrrelevant)
				{
					bLightMapped = false;
				}

				if (InteractionType != LIT_Dynamic && InteractionType != LIT_CachedSignedDistanceFieldShadowMap2D)
				{
					bDynamic = false;
				}

				if (InteractionType != LIT_CachedSignedDistanceFieldShadowMap2D)
				{
					bShadowMapped = false;
				}
			}
		}
	}
	else
	{
		bRelevant = true;
		bLightMapped = false;
		bShadowMapped = false;
	}
}

/** Initialization constructor. */
FStaticMeshSceneProxy::FLODInfo::FLODInfo(const UStaticMeshComponent* InComponent,int32 LODIndex):
	OverrideColorVertexBuffer(0),
	LightMap(NULL),
	ShadowMap(NULL),
	bUsesMeshModifyingMaterials(false)
{
	FStaticMeshRenderData* RenderData = InComponent->StaticMesh->RenderData;
	if(LODIndex < InComponent->LODData.Num())
	{
		const FStaticMeshComponentLODInfo& ComponentLODInfo = InComponent->LODData[LODIndex];

		// Determine if the LOD has static lighting.
		LightMap = ComponentLODInfo.LightMap;
		ShadowMap = ComponentLODInfo.ShadowMap;
		IrrelevantLights = InComponent->IrrelevantLights;

		// Initialize this LOD's overridden vertex colors, if it has any
		if( ComponentLODInfo.OverrideVertexColors )
		{
			FStaticMeshLODResources& LODRenderData = RenderData->LODResources[LODIndex];
			
			// the instance should point to the loaded data to avoid copy and memory waste
			OverrideColorVertexBuffer = ComponentLODInfo.OverrideVertexColors;

			// Setup our vertex factory that points to our overridden color vertex stream.  We'll use this
			// vertex factory when rendering the static mesh instead of it's stock factory
			OverrideColorVertexFactory.Reset( new FLocalVertexFactory() );
			LODRenderData.InitVertexFactory( *OverrideColorVertexFactory.GetOwnedPointer(), InComponent->StaticMesh, OverrideColorVertexBuffer );

			// @todo MeshPaint: Make sure this is the best place to do this; also make sure cleanup is called!
			BeginInitResource( OverrideColorVertexFactory.GetOwnedPointer() );
		}
	}

	if (RenderData->bLODsShareStaticLighting && InComponent->LODData.IsValidIndex(0))
	{
		const FStaticMeshComponentLODInfo& ComponentLODInfo = InComponent->LODData[0];
		LightMap = ComponentLODInfo.LightMap;
		ShadowMap = ComponentLODInfo.ShadowMap;
	}

	bool bHasStaticLighting = LightMap != NULL || ShadowMap != NULL;

	// Gather the materials applied to the LOD.
	Sections.Empty(RenderData->LODResources[LODIndex].Sections.Num());
	FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
	for(int32 SectionIndex = 0;SectionIndex < LODModel.Sections.Num();SectionIndex++)
	{
		const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
		FSectionInfo SectionInfo;

		// Determine the material applied to this element of the LOD.
		SectionInfo.Material = InComponent->GetMaterial(Section.MaterialIndex);
		if (GForceDefaultMaterial && SectionInfo.Material && !IsTranslucentBlendMode(SectionInfo.Material->GetBlendMode()))
		{
			SectionInfo.Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		// If there isn't an applied material, or if we need static lighting and it doesn't support it, fall back to the default material.
		if(!SectionInfo.Material || (bHasStaticLighting && !SectionInfo.Material->CheckMaterialUsage_Concurrent(MATUSAGE_StaticLighting)))
		{
			SectionInfo.Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		const bool bRequiresAdjacencyInformation = RequiresAdjacencyInformation( SectionInfo.Material, LODModel.VertexFactory.GetType() );
		if ( bRequiresAdjacencyInformation && !LODModel.bHasAdjacencyInfo )
		{
			UE_LOG(LogStaticMesh, Warning, TEXT("Adjacency information not built for static mesh with a material that requires it. Using default material instead.\n\tMaterial: %s\n\tStaticMesh: %s"),
				*SectionInfo.Material->GetPathName(), 
				*InComponent->StaticMesh->GetPathName() );
			SectionInfo.Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		// Per-section selection for the editor.
#if WITH_EDITORONLY_DATA
		if (GIsEditor)
		{
			SectionInfo.bSelected = (InComponent->SelectedEditorSection == SectionIndex);
		}
#endif

		// Store the element info.
		Sections.Add(SectionInfo);

		// Flag the entire LOD if any material modifies its mesh
		UMaterialInterface::TMicRecursionGuard RecursionGuard;
		FMaterialResource const* MaterialResource = const_cast<UMaterialInterface const*>(SectionInfo.Material)->GetMaterial_Concurrent(RecursionGuard)->GetMaterialResource(GRHIFeatureLevel);
		if(MaterialResource)
		{
			if(MaterialResource->MaterialModifiesMeshPosition())
			{
				bUsesMeshModifyingMaterials = true;
			}
		}
	}

}



/** Destructor */
FStaticMeshSceneProxy::FLODInfo::~FLODInfo()
{
	if( OverrideColorVertexFactory.IsValid() )
	{
		OverrideColorVertexFactory->ReleaseResource();
		OverrideColorVertexFactory.Reset();
	}

	// delete for OverrideColorVertexBuffer is not required , FStaticMeshComponentLODInfo handle the release of the memory
}

// FLightCacheInterface.
FLightInteraction FStaticMeshSceneProxy::FLODInfo::GetInteraction(const FLightSceneProxy* LightSceneProxy) const
{
	// Check if the light has static lighting or shadowing.
	// This directly accesses the component's static lighting with the assumption that it won't be changed without synchronizing with the rendering thread.
	if (LightSceneProxy->HasStaticShadowing())
	{
		const FGuid LightGuid = LightSceneProxy->GetLightGuid();
		
		if (LightMap && LightMap->ContainsLight(LightGuid))
		{
			return FLightInteraction::LightMap();
		}

		if (ShadowMap && ShadowMap->ContainsLight(LightGuid))
		{
			return FLightInteraction::ShadowMap2D();
		}

		if (IrrelevantLights.Contains(LightGuid))
		{
			return FLightInteraction::Irrelevant();
		}
	}

	// Use dynamic lighting if the light doesn't have static lighting.
	return FLightInteraction::Dynamic();
}

float FStaticMeshSceneProxy::GetMinLODDist(int32 LODIndex) const 
{
	return RenderData->LODDistance[LODIndex];
}

float FStaticMeshSceneProxy::GetMaxLODDist(int32 LODIndex) const 
{
	return RenderData->LODDistance[LODIndex+1];
}

/**
 * Returns the LOD that the primitive will render at for this view. 
 *
 * @param Distance - distance from the current view to the component's bound origin
 */
int32 FStaticMeshSceneProxy::GetLOD(const FSceneView* View) const 
{
	int32 CVarForcedLODLevel = GetCVarForceLOD();

	//If a LOD is being forced, use that one
	if (CVarForcedLODLevel >= 0)
	{
		return FMath::Clamp<int32>(CVarForcedLODLevel, 0, RenderData->LODResources.Num() - 1);
	}

	if (ForcedLodModel > 0)
	{
		return FMath::Clamp(ForcedLodModel, 1, RenderData->LODResources.Num()) - 1;
	}

#if WITH_EDITOR
	if (View->Family && View->Family->EngineShowFlags.LOD == 0)
	{
		return 0;
	}
#endif

	// Note: These distance calculations must match up with the main renderer!
#if !WITH_EDITOR
	const FVector ViewOriginForDistance = View->ViewMatrices.ViewOrigin;
#else
	const FVector ViewOriginForDistance = View->IsPerspectiveProjection() ? View->ViewMatrices.ViewOrigin : View->OverrideLODViewOrigin;
#endif

	float DistanceSquared = (GetBounds().Origin - ViewOriginForDistance).SizeSquared();

	for(int32 LODIndex = LODs.Num() - 1; LODIndex >= 0; LODIndex--)
	{
		// Use the same distances as FStaticMeshSceneProxy::DrawStaticElements 
		// To ensure that LODs change the same way when drawn in a static draw list or when rendered through DrawDynamicElements
		const float MinDist = GetMinLODDist(LODIndex);
		const float MaxDist = GetMaxLODDist(LODIndex);
		
		const float LODFactorDistanceSquared = DistanceSquared * FMath::Square(View->LODDistanceFactor);
		if (LODFactorDistanceSquared >= FMath::Square(MinDist) && LODFactorDistanceSquared < FMath::Square(MaxDist))
		{
			return LODIndex;
		}
	}
	return INDEX_NONE;
}

FPrimitiveSceneProxy* UStaticMeshComponent::CreateSceneProxy()
{
	if(StaticMesh == NULL
		|| StaticMesh->RenderData == NULL
		|| StaticMesh->RenderData->LODResources.Num() == 0
		|| StaticMesh->RenderData->LODResources[0].VertexBuffer.GetNumVertices() == 0)
	{
		return NULL;
	}

#if WITH_EDITOR
	FPrimitiveSceneProxy* Proxy = ::new FStaticMeshSceneProxy(this);
	if (GIsEditor && Proxy)
	{
		SetupLightmapResolutionViewInfo(*Proxy);
	}
	return Proxy;
#else
	//@todo: figure out why i need a ::new (gcc3-specific)
	return ::new FStaticMeshSceneProxy(this);
#endif
}

bool UStaticMeshComponent::ShouldRecreateProxyOnUpdateTransform() const
{
	return (Mobility != EComponentMobility::Movable);
}
