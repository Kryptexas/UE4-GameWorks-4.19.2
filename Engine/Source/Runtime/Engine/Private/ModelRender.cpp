// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ModelRender.cpp: Unreal model rendering
=============================================================================*/

#include "EnginePrivate.h"
#include "LevelUtils.h"
#include "HModel.h"


namespace
{
	/** Returns true if a surface should be drawn. This only affects dynamic drawing for selection. */
	FORCEINLINE bool ShouldDrawSurface(const FBspSurf& Surf)
	{
#if WITH_EDITOR
		// Don't draw portal polygons or those hidden within the editor.
		return (Surf.PolyFlags & PF_Portal) == 0 && !Surf.IsHiddenEd();
#else
		return (Surf.PolyFlags & PF_Portal) == 0;
#endif
	}

}	// anon namespace

/*-----------------------------------------------------------------------------
FModelVertexBuffer
-----------------------------------------------------------------------------*/

FModelVertexBuffer::FModelVertexBuffer(UModel* InModel)
:	Vertices(true)
,	Model(InModel)
{}

void FModelVertexBuffer::InitRHI()
{
	// Calculate the buffer size.
	NumVerticesRHI = Vertices.Num();
	uint32 Size = Vertices.GetResourceDataSize();
	if( Size > 0 )
	{
		// Create the buffer.
		VertexBufferRHI = RHICreateVertexBuffer(Size,&Vertices,BUF_Static);
	}
}

/**
* Serializer for this class
* @param Ar - archive to serialize to
* @param B - data to serialize
*/
FArchive& operator<<(FArchive& Ar,FModelVertexBuffer& B)
{
	B.Vertices.BulkSerialize(Ar);
	return Ar;
}

/*-----------------------------------------------------------------------------
UModelComponent
-----------------------------------------------------------------------------*/


void UModelComponent::BuildRenderData()
{
	UModel* TheModel = GetModel();

#ifdef WITH_EDITOR
	const ULevel* Level = CastChecked<ULevel>(GetOuter());
	const bool bIsGameWorld = !Level || !Level->OwningWorld || Level->OwningWorld->IsGameWorld();
#endif

	// Build the component's index buffer and compute each element's bounding box.
	for(int32 ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
	{
		FModelElement& Element = Elements[ElementIndex];

		// Find the index buffer for the element's material.
		TScopedPointer<FRawIndexBuffer16or32>* IndexBufferRef = TheModel->MaterialIndexBuffers.Find(Element.Material);
		if(!IndexBufferRef)
		{
			IndexBufferRef = &TheModel->MaterialIndexBuffers.Emplace(Element.Material,new FRawIndexBuffer16or32());
		}
		FRawIndexBuffer16or32* const IndexBuffer = *IndexBufferRef;
		check(IndexBuffer);

		Element.IndexBuffer = IndexBuffer;
		Element.FirstIndex = IndexBuffer->Indices.Num();
		Element.NumTriangles = 0;
		Element.MinVertexIndex = 0xffffffff;
		Element.MaxVertexIndex = 0;
		Element.BoundingBox.Init();
		for(int32 NodeIndex = 0;NodeIndex < Element.Nodes.Num();NodeIndex++)
		{
			const uint16& NodeIdx = Element.Nodes[NodeIndex];
			checkf( TheModel->Nodes.IsValidIndex( NodeIdx ), TEXT( "Invalid Node Index, Idx:%d, Num:%d" ), NodeIdx, TheModel->Nodes.Num() );
			const FBspNode& Node = TheModel->Nodes[NodeIdx];
			checkf( TheModel->Surfs.IsValidIndex( Node.iSurf ), TEXT( "Invalid Surf Index, Idx:%d, Num:%d" ), Node.iSurf, TheModel->Surfs.Num() );
			const FBspSurf& Surf = TheModel->Surfs[Node.iSurf];

#if WITH_EDITOR
			// If we're not in a game world, check the surface visibility
			if (!bIsGameWorld && !ShouldDrawSurface(Surf))
			{
				continue;
			}
#endif

			// Don't put portal polygons in the static index buffer.
			if (Surf.PolyFlags & PF_Portal)
				continue;

			for(uint32 BackFace = 0;BackFace < (uint32)((Surf.PolyFlags & PF_TwoSided) ? 2 : 1);BackFace++)
			{
				for(int32 VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
				{
					Element.BoundingBox += TheModel->Points[TheModel->Verts[Node.iVertPool + VertexIndex].pVertex];
				}

				for(int32 VertexIndex = 2;VertexIndex < Node.NumVertices;VertexIndex++)
				{
					IndexBuffer->Indices.Add(Node.iVertexIndex + Node.NumVertices * BackFace);
					IndexBuffer->Indices.Add(Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex);
					IndexBuffer->Indices.Add(Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex - 1);
					Element.NumTriangles++;
				}
				Element.MinVertexIndex = FMath::Min(Node.iVertexIndex + Node.NumVertices * BackFace,Element.MinVertexIndex);
				Element.MaxVertexIndex = FMath::Max(Node.iVertexIndex + Node.NumVertices * BackFace + Node.NumVertices - 1,Element.MaxVertexIndex);
			}
		}

		IndexBuffer->Indices.Shrink();
	}
}

/**
 * A model component scene proxy.
 */
class FModelSceneProxy : public FPrimitiveSceneProxy
{
public:

	FModelSceneProxy(UModelComponent* InComponent):
		FPrimitiveSceneProxy(InComponent),
		Component(InComponent),
		LevelColor(255,255,255),
		PropertyColor(255,255,255),
		CollisionResponse(InComponent->GetCollisionResponseToChannels())
#if WITH_EDITOR
		, CollisionMaterialInstance(GEngine->ShadedLevelColorationUnlitMaterial->GetRenderProxy(false, false),FColor(157,149,223,255)	)
#endif
	{
		OverrideOwnerName(NAME_BSP);
		const TIndirectArray<FModelElement>& SourceElements = Component->GetElements();

		Elements.Empty(SourceElements.Num());
		for(int32 ElementIndex = 0;ElementIndex < SourceElements.Num();ElementIndex++)
		{
			const FModelElement& SourceElement = SourceElements[ElementIndex];
			FElementInfo* Element = new(Elements) FElementInfo(SourceElement);
			MaterialRelevance |= Element->GetMaterial()->GetRelevance();
		}

		// Try to find a color for level coloration.
		UObject* ModelOuter = InComponent->GetModel()->GetOuter();
		ULevel* Level = Cast<ULevel>( ModelOuter );
		if ( Level )
		{
			ULevelStreaming* LevelStreaming = FLevelUtils::FindStreamingLevel( Level );
			if ( LevelStreaming )
			{
				LevelColor = LevelStreaming->DrawColor;
			}
		}

		// Get a color for property coloration.
		GEngine->GetPropertyColorationColor( (UObject*)InComponent, PropertyColor );
	}

	bool IsCollisionView(const FSceneView* View, bool & bDrawCollision) const
	{
		const bool bInCollisionView = View->Family->EngineShowFlags.CollisionVisibility || View->Family->EngineShowFlags.CollisionPawn;
		if (bInCollisionView)
		{
			// use wireframe if collision is enabled, and it's not using complex 
			bDrawCollision = View->Family->EngineShowFlags.CollisionPawn && IsCollisionEnabled() && (CollisionResponse.GetResponse(ECC_Pawn) != ECR_Ignore);
			bDrawCollision |= View->Family->EngineShowFlags.CollisionVisibility && IsCollisionEnabled() && (CollisionResponse.GetResponse(ECC_Visibility) != ECR_Ignore);
		}
		else
		{
			bDrawCollision = false;
		}

		return bInCollisionView;
	}

	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent*,TArray<TRefCountPtr<HHitProxy> >& OutHitProxies)
	{
		HHitProxy* ModelHitProxy = new HModel(CastChecked<UModelComponent>(Component), Component->GetModel());
		OutHitProxies.Add(ModelHitProxy);
		return ModelHitProxy;
	}

	virtual void PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber) OVERRIDE
	{
		// Reset any batches leftover from last frame.
		PerViewBatches.Reset();
		DynamicMeshBatches.Reset();
		bool bAnySelectedSurfs = false;

		int32 ViewBit = 1;
		int32 NumViews = ViewFamily->Views.Num();
		for (int32 ViewIndex = 0; ViewIndex < NumViews; ++ViewIndex, ViewBit <<= 1)
		{
			// Create a batch entry for each view whether this primitive is visible in it or not!
			FPerViewBatch* ViewBatch = new(PerViewBatches) FPerViewBatch();

			const FSceneView* View = ViewFamily->Views[ViewIndex];
			bool bVisibleInView = (VisibilityMap & ViewBit) == ViewBit;
			bool bShowSelection = GIsEditor && !View->bIsGameView && View->Family->EngineShowFlags.Selection;
			bool bDynamicBSPTriangles = bShowSelection || IsRichView(View) ;
			bool bShowBSPTriangles = View->Family->EngineShowFlags.BSPTriangles;
			bool bShowBSP = View->Family->EngineShowFlags.BSP;

#if WITH_EDITOR
			bool bDrawCollision = false;
			const bool bInCollisionView = IsCollisionView(View, bDrawCollision);
			// draw bsp as dynamic when in collision view mode
			if(bInCollisionView)
			{
				bDynamicBSPTriangles = true;
			}
#endif

			// If in a collision view, only draw if we have collision
			if (bVisibleInView && bDynamicBSPTriangles && bShowBSPTriangles && bShowBSP 
#if WITH_EDITOR
				&& (!bInCollisionView || bDrawCollision)
#endif
				)
			{
				ViewBatch->FirstBatch = DynamicMeshBatches.Num();
				ESceneDepthPriorityGroup DepthPriorityGroup = (ESceneDepthPriorityGroup)GetDepthPriorityGroup(View);

				const FMaterialRenderProxy* MatProxyOverride = NULL;

#if WITH_EDITOR
				if(bInCollisionView && AllowDebugViewmodes())
				{
					MatProxyOverride = &CollisionMaterialInstance;
				}
#endif

				// If selection is being shown, batch triangles based on whether they are selected or not.
				if (bShowSelection)
				{
					uint32 TotalIndices = 0;
					for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
					{
						const FModelElement& ModelElement = Component->GetElements()[ElementIndex];
						TotalIndices += ModelElement.NumTriangles * 3;
					}

					if (TotalIndices > 0)
					{
						FGlobalDynamicIndexBuffer::FAllocation IndexAllocation =
							FGlobalDynamicIndexBuffer::Get().Allocate<uint32>(TotalIndices);
						if (IndexAllocation.IsValid())
						{
							uint32* Indices = (uint32*)IndexAllocation.Buffer;
							uint32 FirstIndex = IndexAllocation.FirstIndex;
							for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
							{
								const FModelElement& ModelElement = Component->GetElements()[ElementIndex];
								if (ModelElement.NumTriangles > 0)
								{
									const FElementInfo& ProxyElementInfo = Elements[ElementIndex];
									bool bHasSelectedSurfs = false;
									bool bHasHoveredSurfs = false;
									for (uint32 BatchIndex = 0; BatchIndex < 3; BatchIndex++)
									{
										// Three batches total:
										//		Batch 0: Only surfaces that are neither selected, nor hovered
										//		Batch 1: Only selected surfaces
										//		Batch 2: Only hovered surfaces
										const bool bOnlySelectedSurfaces = ( BatchIndex == 1 );
										const bool bOnlyHoveredSurfaces = ( BatchIndex == 2 );

										if (bOnlySelectedSurfaces && !bHasSelectedSurfs)
										{
											continue;
										}

										if (bOnlyHoveredSurfaces && !bHasHoveredSurfs)
										{
											continue;
										}

										uint32 MinVertexIndex = MAX_uint32;
										uint32 MaxVertexIndex = 0;
										uint32 NumIndices = 0;

										for(int32 NodeIndex = 0;NodeIndex < ModelElement.Nodes.Num();NodeIndex++)
										{
											FBspNode& Node = Component->GetModel()->Nodes[ModelElement.Nodes[NodeIndex]];
											FBspSurf& Surf = Component->GetModel()->Surfs[Node.iSurf];

											if (!ShouldDrawSurface(Surf))
												continue;

											const bool bSurfaceSelected = (Surf.PolyFlags & PF_Selected) == PF_Selected;
											const bool bSurfaceHovered = !bSurfaceSelected && ((Surf.PolyFlags & PF_Hovered) == PF_Hovered);
											bHasSelectedSurfs |= bSurfaceSelected;
											bHasHoveredSurfs |= bSurfaceHovered;

											if (bSurfaceSelected == bOnlySelectedSurfaces && bSurfaceHovered == bOnlyHoveredSurfaces)
											{
												for (uint32 BackFace = 0; BackFace < (uint32)((Surf.PolyFlags & PF_TwoSided) ? 2 : 1); BackFace++)
												{
													for (int32 VertexIndex = 2; VertexIndex < Node.NumVertices; VertexIndex++)
													{
														*Indices++ = Node.iVertexIndex + Node.NumVertices * BackFace;
														*Indices++ = Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex;
														*Indices++ = Node.iVertexIndex + Node.NumVertices * BackFace + VertexIndex - 1;
														NumIndices += 3;
													}
													MinVertexIndex = FMath::Min(Node.iVertexIndex + Node.NumVertices * BackFace,MinVertexIndex);
													MaxVertexIndex = FMath::Max(Node.iVertexIndex + Node.NumVertices * BackFace + Node.NumVertices - 1,MaxVertexIndex);
												}
											}
										}

										if (NumIndices > 0)
										{
											FDynamicModelMeshBatch& MeshElement = *new(DynamicMeshBatches) FDynamicModelMeshBatch(bOnlySelectedSurfaces);
											FMeshBatchElement& BatchElement = MeshElement.Elements[0];
											BatchElement.IndexBuffer = IndexAllocation.IndexBuffer;
											MeshElement.VertexFactory = &Component->GetModel()->VertexFactory;
											MeshElement.MaterialRenderProxy = (MatProxyOverride != NULL) ? MatProxyOverride : ProxyElementInfo.GetMaterial()->GetRenderProxy(bOnlySelectedSurfaces, bOnlyHoveredSurfaces);
											MeshElement.LCI = &ProxyElementInfo;
											BatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
											BatchElement.FirstIndex = FirstIndex;
											BatchElement.NumPrimitives = NumIndices / 3;
											BatchElement.MinVertexIndex = MinVertexIndex;
											BatchElement.MaxVertexIndex = MaxVertexIndex;
											MeshElement.Type = PT_TriangleList;
											MeshElement.DepthPriorityGroup = DepthPriorityGroup;
											MeshElement.ModelElementIndex = ElementIndex;
											FirstIndex += NumIndices;
										}
									}
									bAnySelectedSurfs |= bHasSelectedSurfs;
								}
							}
						}
					}
				}
				else
				{
					for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
					{
						const FModelElement& ModelElement = Component->GetElements()[ElementIndex];
						if (ModelElement.NumTriangles > 0)
						{
							FDynamicModelMeshBatch& MeshElement = *new(DynamicMeshBatches) FDynamicModelMeshBatch(false);
							FMeshBatchElement& BatchElement = MeshElement.Elements[0];
							BatchElement.IndexBuffer = ModelElement.IndexBuffer;
							MeshElement.VertexFactory = &Component->GetModel()->VertexFactory;
							MeshElement.MaterialRenderProxy = (MatProxyOverride != NULL) ? MatProxyOverride : Elements[ElementIndex].GetMaterial()->GetRenderProxy(false);
							MeshElement.LCI = &Elements[ElementIndex];
							BatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
							BatchElement.FirstIndex = ModelElement.FirstIndex;
							BatchElement.NumPrimitives = ModelElement.NumTriangles;
							BatchElement.MinVertexIndex = ModelElement.MinVertexIndex;
							BatchElement.MaxVertexIndex = ModelElement.MaxVertexIndex;
							MeshElement.Type = PT_TriangleList;
							MeshElement.DepthPriorityGroup = DepthPriorityGroup;
							MeshElement.ModelElementIndex = ElementIndex;
						}
					}
				}

				ViewBatch->LastBatch = DynamicMeshBatches.Num();
			}
		}

		SetSelection_RenderThread(bAnySelectedSurfs);
	}

	/** 
	* Draw the scene proxy as a dynamic element
	*
	* @param	PDI - draw interface to render to
	* @param	View - current view
	*/
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
		QUICK_SCOPE_CYCLE_COUNTER( STAT_ModelSceneProxy_DrawDynamicElements );

		int32 ViewIndex = View->Family->Views.Find(View);
		const bool bRenderingSelectionOutline = PDI->IsRenderingSelectionOutline();

		if (PerViewBatches.IsValidIndex(ViewIndex))
		{
			FLinearColor UtilColor = LevelColor;

			const bool bIsWireframe = View->Family->EngineShowFlags.Wireframe;
			FPerViewBatch& PerViewBatch = PerViewBatches[ViewIndex];
			for (int32 BatchIndex = PerViewBatch.FirstBatch; BatchIndex < PerViewBatch.LastBatch; ++BatchIndex)
			{
				FDynamicModelMeshBatch& Batch = DynamicMeshBatches[BatchIndex];
				if (!bRenderingSelectionOutline || Batch.bIsSelectedBatch)
				{
					if (Batch.ModelElementIndex < ElementLightMapResolutions.Num())
					{
						SetLightMapResolutionScale(ElementLightMapResolutions[Batch.ModelElementIndex]);
					}
					else
					{
						SetLightMapResolutionScale(FVector2D(0.0f, 0.0f));
					}

					DrawRichMesh(PDI,Batch,FLinearColor::White,UtilColor,PropertyColor,this,false, bIsWireframe);
				}
			}
		}
	}

	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI)
	{
		if(!HasViewDependentDPG())
		{
			// Determine the DPG the primitive should be drawn in.
			uint8 PrimitiveDPG = GetStaticDepthPriorityGroup();

			const FMatrix LocalToWorld = Component->GetRenderMatrix();

			for(int32 ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
			{
				const FModelElement& ModelElement = Component->GetElements()[ElementIndex];
				if(ModelElement.NumTriangles > 0)
				{
					FMeshBatch MeshElement;
					FMeshBatchElement& BatchElement = MeshElement.Elements[0];
					BatchElement.IndexBuffer = ModelElement.IndexBuffer;
					MeshElement.VertexFactory = &Component->GetModel()->VertexFactory;
					MeshElement.MaterialRenderProxy = Elements[ElementIndex].GetMaterial()->GetRenderProxy(false);
					MeshElement.LCI = &Elements[ElementIndex];
					BatchElement.PrimitiveUniformBufferResource = &GetUniformBuffer();
					BatchElement.FirstIndex = ModelElement.FirstIndex;
					BatchElement.NumPrimitives = ModelElement.NumTriangles;
					BatchElement.MinVertexIndex = ModelElement.MinVertexIndex;
					BatchElement.MaxVertexIndex = ModelElement.MaxVertexIndex;
					MeshElement.Type = PT_TriangleList;
					MeshElement.DepthPriorityGroup = PrimitiveDPG;
					PDI->DrawMesh(MeshElement,0,FLT_MAX);
				}
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.BSPTriangles && View->Family->EngineShowFlags.BSP;
		bool bShowSelectedTriangles = GIsEditor && !View->bIsGameView && View->Family->EngineShowFlags.Selection;
		bool bCollisionView = View->Family->EngineShowFlags.CollisionPawn || View->Family->EngineShowFlags.CollisionVisibility;
		if (IsRichView(View) || HasViewDependentDPG() || bCollisionView
			|| (bShowSelectedTriangles && HasSelectedSurfaces()))
		{
			Result.bDynamicRelevance = true;
			Result.bNeedsPreRenderView = true;
		}
		else
		{
			Result.bStaticRelevance = true;
		}
		Result.bShadowRelevance = IsShadowCast(View);
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual bool CanBeOccluded() const OVERRIDE
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const
	{
		// Attach the light to the primitive's static meshes.
		bDynamic = true;
		bRelevant = false;
		bLightMapped = true;
		bShadowMapped = true;

		if (Elements.Num() > 0)
		{
			for(int32 ElementIndex = 0;ElementIndex < Elements.Num();ElementIndex++)
			{
				const FElementInfo* LCI = &Elements[ElementIndex];
				if (LCI)
				{
					ELightInteractionType InteractionType = LCI->GetInteraction(LightSceneProxy).GetType();
					if(InteractionType != LIT_CachedIrrelevant)
					{
						bRelevant = true;
						if(InteractionType != LIT_CachedLightMap)
						{
							bLightMapped = false;
						}
						if(InteractionType != LIT_Dynamic && InteractionType != LIT_CachedSignedDistanceFieldShadowMap2D)
						{
							bDynamic = false;
						}
					}
				}
			}
		}
		else
		{
			bRelevant = true;
			bLightMapped = false;
		}
	}

	virtual uint32 GetMemoryFootprint( void ) const { return( sizeof( *this ) + GetAllocatedSize() ); }
	uint32 GetAllocatedSize( void ) const 
	{ 
		uint32 AdditionalSize = FPrimitiveSceneProxy::GetAllocatedSize();

		AdditionalSize += Elements.GetAllocatedSize();

		return( AdditionalSize ); 
	}

	virtual bool ShowInBSPSplitViewmode() const
	{
		return true;
	}

private:

	/** Returns true if any surfaces relevant to this component are selected (or hovered). */
	bool HasSelectedSurfaces() const
	{
		for (int32 ElementIndex = 0; ElementIndex < Elements.Num(); ElementIndex++)
		{
			const FModelElement& ModelElement = Component->GetElements()[ElementIndex];
			if (ModelElement.NumTriangles > 0)
			{
				for(int32 NodeIndex = 0;NodeIndex < ModelElement.Nodes.Num();NodeIndex++)
				{
					FBspNode& Node = Component->GetModel()->Nodes[ModelElement.Nodes[NodeIndex]];
					FBspSurf& Surf = Component->GetModel()->Surfs[Node.iSurf];

					if (ShouldDrawSurface(Surf) && (Surf.PolyFlags & (PF_Selected | PF_Hovered)) != 0)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	UModelComponent* Component;

	class FElementInfo: public FLightCacheInterface
	{
	public:

		/** Initialization constructor. */
		FElementInfo(const FModelElement& InModelElement)
		:	ModelElement(InModelElement)
		,	Bounds(ModelElement.BoundingBox)
		{
			const bool bHasStaticLighting = ModelElement.LightMap != NULL || ModelElement.ShadowMap != NULL;

			// Determine the material applied to the model element.
			Material = ModelElement.Material;

			// If there isn't an applied material, or if we need static lighting and it doesn't support it, fall back to the default material.
			if(!ModelElement.Material || (bHasStaticLighting && !ModelElement.Material->CheckMaterialUsage(MATUSAGE_StaticLighting)))
			{
				Material = UMaterial::GetDefaultMaterial(MD_Surface);
			}

			LightMap = ModelElement.LightMap;
			ShadowMap = ModelElement.ShadowMap;
		}
		
		// FLightCacheInterface.
		virtual FLightInteraction GetInteraction(const FLightSceneProxy* LightSceneProxy) const
		{
			if( LightSceneProxy->HasStaticShadowing() )
			{
				if( ModelElement.IrrelevantLights.Contains( LightSceneProxy->GetLightGuid() ) )
				{
					return FLightInteraction::Irrelevant();
				}

				if( LightMap && LightMap->ContainsLight( LightSceneProxy->GetLightGuid() ) )
				{
					return FLightInteraction::LightMap();
				}

				if( ShadowMap && ShadowMap->ContainsLight( LightSceneProxy->GetLightGuid() ) )
				{
					return FLightInteraction::ShadowMap2D();
				}
			}

			// Cull the uncached light against the bounding box of the element.
			if(	LightSceneProxy->AffectsBounds(Bounds) )
			{
				return FLightInteraction::Dynamic();
			}
			else
			{
				return FLightInteraction::Irrelevant();
			}
		}

		virtual FLightMapInteraction GetLightMapInteraction() const
		{
			return LightMap ? LightMap->GetInteraction() : FLightMapInteraction();
		}

		virtual FShadowMapInteraction GetShadowMapInteraction() const
		{
			return ShadowMap ? ShadowMap->GetInteraction() : FShadowMapInteraction();
		}

		// Accessors.
		UMaterialInterface* GetMaterial() const { return Material; }
		/** Associated model element. */
		const FModelElement* GetModelElement() const { return &ModelElement; }

	private:

		/** The element's material. */
		UMaterialInterface* Material;

		/** Associated model element. */
		const FModelElement& ModelElement;

		/** The light-map used by the element. */
		const FLightMap* LightMap;

		const FShadowMap* ShadowMap;

		/** The element's bounding volume. */
		FBoxSphereBounds Bounds;
	};

	TArray<FElementInfo> Elements;
	TArray<FVector2D> ElementLightMapResolutions;

	FColor LevelColor;
	FColor PropertyColor;

	FMaterialRelevance MaterialRelevance;

	/** Which dynamic batches should be rendered for a given view. */
	struct FPerViewBatch
	{
		int32 FirstBatch;
		int32 LastBatch;

		FPerViewBatch()
			: FirstBatch(0)
			, LastBatch(0)
		{
		}
	};
	TArray<FPerViewBatch, TInlineAllocator<2> > PerViewBatches;

	/** Precomputed dynamic mesh batches. */
	struct FDynamicModelMeshBatch : public FMeshBatch
	{
		int32 ModelElementIndex;
		/** true if the batch is selected (we need to override the material)*/
		bool bIsSelectedBatch;
		FDynamicModelMeshBatch( bool bInIsSelectedBatch )
			: FMeshBatch()
			, ModelElementIndex(0)
			, bIsSelectedBatch( bInIsSelectedBatch )
		{
		}
	};
	TArray<FDynamicModelMeshBatch> DynamicMeshBatches;

	/** Collision Response of this component**/
	FCollisionResponseContainer CollisionResponse;
#if WITH_EDITOR
	/** Material proxy to use when rendering in a collision view mode */
	FColoredMaterialRenderProxy CollisionMaterialInstance;
#endif
public:
	// Helper functions for LightMap Density view mode
	/**
	 *	Get the number of entires in the Elements array.
	 *
	 *	@return	int32		The number of entries in the array.
	 */
	int32 GetElementCount() const { return Elements.Num(); }

	/**
	 *	Get the element info at the given index.
	 *
	 *	@param	Index			The index of interest
	 *
	 *	@return	FElementInfo*	The element info at that index.
	 *							NULL if out of range.
	 */
	const FElementInfo* GetElement(int32 Index) const 
	{
		if (Index < Elements.Num())
		{
			return &(Elements[Index]);
		}
		return NULL;
	}

	/**
	 *	Clear the element LightMap resolutions array.
	 */
	void ClearElementLightMapResolutions()
	{
		ElementLightMapResolutions.Empty();
	}

	void AddElementLightMapResolution(int32 InElementIdx, int32 InSizeX, int32 InSizeY)
	{
		if (ElementLightMapResolutions.Num() <= InElementIdx)
		{
			ElementLightMapResolutions.AddZeroed(InElementIdx - ElementLightMapResolutions.Num() + 1);
		}

		ElementLightMapResolutions[InElementIdx] = FVector2D((float)InSizeX, (float)InSizeY);
	}

	friend class UModelComponent;
};

FPrimitiveSceneProxy* UModelComponent::CreateSceneProxy()
{
	FPrimitiveSceneProxy* Proxy = ::new FModelSceneProxy(this);
#if WITH_EDITOR
	if (GIsEditor && Proxy)
	{
		SetupLightmapResolutionViewInfo(*Proxy);
	}
#endif
	return Proxy;
}

bool UModelComponent::ShouldRecreateProxyOnUpdateTransform() const
{
	return true;
}

FBoxSphereBounds UModelComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	if(Model)
	{
		FBox	BoundingBox(0);
		for(int32 NodeIndex = 0;NodeIndex < Nodes.Num();NodeIndex++)
		{
			FBspNode& Node = Model->Nodes[Nodes[NodeIndex]];
			for(int32 VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
			{
				BoundingBox += Model->Points[Model->Verts[Node.iVertPool + VertexIndex].pVertex];
			}
		}
		return FBoxSphereBounds(BoundingBox.TransformBy(LocalToWorld));
	}
	else
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}
}

#if WITH_EDITOR
bool UModelComponent::SetupLightmapResolutionViewInfo(FPrimitiveSceneProxy& Proxy) const
{
	FModelSceneProxy* ModelProxy = (FModelSceneProxy*)(&Proxy);

	// Alway texture based...
	ModelProxy->SetLightMapType(LMIT_Texture);
	ModelProxy->SetIsLightMapResolutionPadded(true);

	// Fill in the resolutions array
	ModelProxy->ClearElementLightMapResolutions();

	if (Model->NodeGroups.Num() > 0)
	{
		for (int32 ElementIdx = 0; ElementIdx < ModelProxy->GetElementCount(); ElementIdx++)
		{
			const FModelSceneProxy::FElementInfo* Element = ModelProxy->GetElement(ElementIdx);
			if (Element)
			{
				const FModelElement* ModelElement = Element->GetModelElement();
				if (ModelElement && (ModelElement->Nodes.Num() > 0))
				{
					// All the nodes in this element SHOULD be in the same node group...
					int32 NodeIdx = ModelElement->Nodes[0];

					// Find the node group it belong to
					FNodeGroup* FoundNodeGroup = NULL;
					// find the NodeGroup that this node went into, and get all of its node
					for (TMap<int32, FNodeGroup*>::TIterator It(Model->NodeGroups); It && (FoundNodeGroup == NULL); ++It)
					{
						FNodeGroup* NodeGroup = It.Value();
						check(NodeGroup != NULL);
						for (int32 NodeIndex = 0; NodeIndex < NodeGroup->Nodes.Num(); NodeIndex++)
						{
							if (NodeGroup->Nodes[NodeIndex] == NodeIdx)
							{
								FoundNodeGroup = NodeGroup;
								break;
							}
						}
					}

					ModelProxy->AddElementLightMapResolution(ElementIdx, 
						FoundNodeGroup ? FoundNodeGroup->SizeX : 0, 
						FoundNodeGroup ? FoundNodeGroup->SizeY : 0);
				}
			}
		}
	}

	return true;
}
#endif // WITH_EDITOR
