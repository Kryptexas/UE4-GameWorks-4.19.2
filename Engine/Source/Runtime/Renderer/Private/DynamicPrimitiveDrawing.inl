// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DynamicPrimitiveDrawing.inl: Dynamic primitive drawing implementation.
=============================================================================*/

#ifndef __DYNAMICPRIMITIVEDRAWING_INL__
#define __DYNAMICPRIMITIVEDRAWING_INL__

template<typename DrawingPolicyFactoryType>
TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::~TDynamicPrimitiveDrawer()
{
	if(View)
	{
		const bool bNeedToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(GRHIShaderPlatform) && !IsMobileHDR();

		// Determine whether or not to depth test simple batched elements manually (msaa only)
		// We need to read from the scene depth in the pixel shader to do manual depth testing
		// Hardware depth testing will not work because there are two depth buffers (in the msaa case)
		FTexture2DRHIRef DepthTexture;
		if( bEditorCompositeDepthTest )
		{
			DepthTexture = GSceneRenderTargets.GetSceneDepthTexture();
		}
		
		// Draw the batched elements.
		BatchedElements.Draw(
			RHICmdList,
			bNeedToSwitchVerticalAxis,
			View->ViewProjectionMatrix,
			View->ViewRect.Width(),
			View->ViewRect.Height(),
			View->Family->EngineShowFlags.HitProxies,
			1.0f,
			View,
			DepthTexture
			);
	}

	// Cleanup the dynamic resources.
	for(int32 ResourceIndex = 0;ResourceIndex < DynamicResources.Num();ResourceIndex++)
	{
		//release the resources before deleting, they will delete themselves
		DynamicResources[ResourceIndex]->ReleasePrimitiveResource();
	}
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::SetPrimitive(const FPrimitiveSceneProxy* NewPrimitiveSceneProxy)
{
	PrimitiveSceneProxy = NewPrimitiveSceneProxy;
	if (NewPrimitiveSceneProxy)
	{
		HitProxyId = PrimitiveSceneProxy->GetPrimitiveSceneInfo()->DefaultDynamicHitProxyId;
	}
}

template<typename DrawingPolicyFactoryType>
bool TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::IsHitTesting()
{
	return bIsHitTesting;
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::SetHitProxy(HHitProxy* HitProxy)
{
	if(HitProxy)
	{
		// Only allow hit proxies from CreateHitProxies.
		checkf(
			PrimitiveSceneProxy->GetPrimitiveSceneInfo()->HitProxies.Find(HitProxy) != INDEX_NONE,
			TEXT("Hit proxy used in DrawDynamicElements which wasn't created in CreateHitProxies")
			);
		HitProxyId = HitProxy->Id;
	}
	else
	{
		HitProxyId = FHitProxyId();
	}
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource)
{
	// Add the dynamic resource to the list of resources to cleanup on destruction.
	DynamicResources.Add(DynamicResource);

	// Initialize the dynamic resource immediately.
	DynamicResource->InitPrimitiveResource();
}

template<typename DrawingPolicyFactoryType>
bool TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy, ERHIFeatureLevel::Type InFeatureLevel) const
{
	return DrawingPolicyFactoryType::IsMaterialIgnored(MaterialRenderProxy, InFeatureLevel);
}

template<typename DrawingPolicyFactoryType>
int32 TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::DrawMesh(const FMeshBatch& Mesh)
{
	int32 NumPassesRendered=0;
	
	check(Mesh.GetNumPrimitives() > 0);
	check(Mesh.VertexFactory);
#if DO_CHECK
	Mesh.CheckUniformBuffers();
#endif
	
	INC_DWORD_STAT_BY(STAT_DynamicPathMeshDrawCalls,Mesh.Elements.Num());
	const bool DrawDirty = DrawingPolicyFactoryType::DrawDynamicMesh(
		RHICmdList,
		*View,
		DrawingContext,
		Mesh,
		false,
		bPreFog,
		PrimitiveSceneProxy,
		HitProxyId
		);
	bDirty |= DrawDirty;

	NumPassesRendered += DrawDirty;
	return NumPassesRendered;
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::DrawSprite(
	const FVector& Position,
	float SizeX,
	float SizeY,
	const FTexture* Sprite,
	const FLinearColor& Color,
	uint8 DepthPriorityGroup,
	float U,
	float UL,
	float V,
	float VL,
	uint8 BlendMode
	)
{
	if(DrawingPolicyFactoryType::bAllowSimpleElements)
	{
		BatchedElements.AddSprite(
			Position,
			SizeX,
			SizeY,
			Sprite,
			Color,
			HitProxyId,
			U,
			UL,
			V,
			VL,
			BlendMode
		);
		bDirty = true;
	}
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::AddReserveLines(uint8 DepthPriorityGroup, int32 NumLines, bool bDepthBiased)
{
	if (DrawingPolicyFactoryType::bAllowSimpleElements)
	{
		BatchedElements.AddReserveLines(NumLines, bDepthBiased);
	}
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::DrawLine(
	const FVector& Start,
	const FVector& End,
	const FLinearColor& Color,
	uint8 DepthPriorityGroup,
	float Thickness/* = 0.0f*/,
	float DepthBias/* = 0.0f*/,
	bool bScreenSpace/* = false*/
	)
{
	if(DrawingPolicyFactoryType::bAllowSimpleElements)
	{
		BatchedElements.AddLine(
			Start,
			End,
			Color,
			HitProxyId,
			Thickness,
			DepthBias,
			bScreenSpace
		);
		bDirty = true;
	}
}

template<typename DrawingPolicyFactoryType>
void TDynamicPrimitiveDrawer<DrawingPolicyFactoryType>::DrawPoint(
	const FVector& Position,
	const FLinearColor& Color,
	float PointSize,
	uint8 DepthPriorityGroup
	)
{
	if(DrawingPolicyFactoryType::bAllowSimpleElements)
	{
		BatchedElements.AddPoint(
			Position,
			PointSize,
			Color,
			HitProxyId
		);
		bDirty = true;
	}
}

template<class DrawingPolicyFactoryType>
void DrawViewElementsInner(
	FRHICommandList& RHICmdList,
	const FViewInfo& View,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	uint8 DPGIndex,
	bool bPreFog,
	int32 FirstIndex,
	int32 LastIndex
	)
{
	// Get the correct element list based on dpg index
	const TIndirectArray<FMeshBatch>& ViewMeshElementList = ( DPGIndex == SDPG_Foreground ? View.TopViewMeshElements : View.ViewMeshElements );
	// Draw the view's mesh elements.
	check(LastIndex < ViewMeshElementList.Num());
	for (int32 MeshIndex = FirstIndex; MeshIndex <= LastIndex; MeshIndex++)
	{
		const FMeshBatch& Mesh = ViewMeshElementList[MeshIndex];
		const auto FeatureLevel = View.GetFeatureLevel();
		check(Mesh.MaterialRenderProxy);
		check(Mesh.MaterialRenderProxy->GetMaterial(FeatureLevel));
		const bool bIsTwoSided = Mesh.MaterialRenderProxy->GetMaterial(FeatureLevel)->IsTwoSided();
		int32 bBackFace = bIsTwoSided ? 1 : 0;
		do
		{
			DrawingPolicyFactoryType::DrawDynamicMesh(
				RHICmdList, 
				View,
				DrawingContext,
				Mesh,
				!!bBackFace,
				bPreFog,
				NULL,
				Mesh.BatchHitProxyId
				);
			--bBackFace;
		} while( bBackFace >= 0 );
	}
}

template<typename DrawingPolicyFactoryType>
class FDrawViewElementsAnyThreadTask
{
	FRHICommandList& RHICmdList;
	const FViewInfo& View;
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext;
	uint8 DPGIndex;
	bool bPreFog;


	const int32 FirstIndex;
	const int32 LastIndex;

public:

	FDrawViewElementsAnyThreadTask(
		FRHICommandList* InRHICmdList,
		const FViewInfo* InView,
		const typename DrawingPolicyFactoryType::ContextType& InDrawingContext,
		uint8 InDPGIndex,
		bool InbPreFog,
		int32 InFirstIndex,
		int32 InLastIndex
		)
		: RHICmdList(*InRHICmdList)
		, View(*InView)
		, DrawingContext(InDrawingContext)
		, DPGIndex(InDPGIndex)
		, bPreFog(InbPreFog)
		, FirstIndex(InFirstIndex)
		, LastIndex(InLastIndex)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FDrawViewElementsAnyThreadTask, STATGROUP_TaskGraphTasks);
	}

	ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		DrawViewElementsInner<DrawingPolicyFactoryType>(RHICmdList, View, DrawingContext, DPGIndex, bPreFog, FirstIndex, LastIndex);
	}
};

template<class DrawingPolicyFactoryType>
void DrawViewElementsParallel(
	const FViewInfo& View,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	uint8 DPGIndex,
	bool bPreFog,
	FRHICommandList& ParentCmdList,
	int32 Width
	)
{
	// Get the correct element list based on dpg index
	const TIndirectArray<FMeshBatch>& ViewMeshElementList = (DPGIndex == SDPG_Foreground ? View.TopViewMeshElements : View.ViewMeshElements);

	{
		int32 NumPrims = ViewMeshElementList.Num();
		int32 EffectiveThreads = FMath::Min<int32>(NumPrims, Width);

		int32 Start = 0;
		if (EffectiveThreads)
		{

			int32 NumPer = NumPrims / EffectiveThreads;
			int32 Extra = NumPrims - NumPer * EffectiveThreads;


			for (int32 ThreadIndex = 0; ThreadIndex < EffectiveThreads; ThreadIndex++)
			{
				int32 Last = Start + (NumPer - 1) + (ThreadIndex < Extra);
				check(Last >= Start);

				{
					FRHICommandList* CmdList = new FRHICommandList;

					FGraphEventRef AnyThreadCompletionEvent = TGraphTask<FDrawViewElementsAnyThreadTask<DrawingPolicyFactoryType> >::CreateTask(nullptr, ENamedThreads::RenderThread)
						.ConstructAndDispatchWhenReady(CmdList, &View, DrawingContext, DPGIndex, bPreFog, Start, Last);

					ParentCmdList.QueueAsyncCommandListSubmit(AnyThreadCompletionEvent, CmdList);
				}
				Start = Last + 1;
			}
		}
	}
}

template<class DrawingPolicyFactoryType>
bool DrawViewElements(
	FRHICommandList& RHICmdList,
	const FViewInfo& View,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	uint8 DPGIndex,
	bool bPreFog
	)
{
	// Get the correct element list based on dpg index
	const TIndirectArray<FMeshBatch>& ViewMeshElementList = (DPGIndex == SDPG_Foreground ? View.TopViewMeshElements : View.ViewMeshElements);
	if (ViewMeshElementList.Num() != 0)
	{
		DrawViewElementsInner<DrawingPolicyFactoryType>(RHICmdList, View, DrawingContext, DPGIndex, bPreFog, 0, ViewMeshElementList.Num() - 1);
		return true;
	}
	return false;
}

template<class DrawingPolicyFactoryType>
bool DrawDynamicPrimitiveSet(
	FRHICommandListImmediate& RHICmdList,
	const FViewInfo& View,
	const TArray<const FPrimitiveSceneInfo*,SceneRenderingAllocator>& PrimitiveSet,
	const typename DrawingPolicyFactoryType::ContextType& DrawingContext,
	bool bPreFog,
	bool bIsHitTesting
	)
{
	
	// Draw the elements of each dynamic primitive.
	TDynamicPrimitiveDrawer<DrawingPolicyFactoryType> Drawer(RHICmdList, &View, DrawingContext, bPreFog, bIsHitTesting);
	for(int32 PrimitiveIndex = 0; PrimitiveIndex < PrimitiveSet.Num(); PrimitiveIndex++)
	{
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSet[PrimitiveIndex];
		int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();

		if(!View.PrimitiveVisibilityMap[PrimitiveId])
		{
			continue;
		}
		FScopeCycleCounter Context(PrimitiveSceneInfo->Proxy->GetStatId());

		Drawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
		PrimitiveSceneInfo->Proxy->DrawDynamicElements(
			&Drawer,
			&View
			);
	}

	return Drawer.IsDirty();
}

inline FViewElementPDI::FViewElementPDI(FViewInfo* InViewInfo,FHitProxyConsumer* InHitProxyConsumer):
	FPrimitiveDrawInterface(InViewInfo),
	ViewInfo(InViewInfo),
	HitProxyConsumer(InHitProxyConsumer)
{}

inline bool FViewElementPDI::IsHitTesting()
{
	return HitProxyConsumer != NULL;
}
inline void FViewElementPDI::SetHitProxy(HHitProxy* HitProxy)
{
	// Change the current hit proxy.
	CurrentHitProxy = HitProxy;

	if(HitProxyConsumer && HitProxy)
	{
		// Notify the hit proxy consumer of the new hit proxy.
		HitProxyConsumer->AddHitProxy(HitProxy);
	}
}

inline void FViewElementPDI::RegisterDynamicResource(FDynamicPrimitiveResource* DynamicResource)
{
	ViewInfo->DynamicResources.Add(DynamicResource);
}

inline FBatchedElements& FViewElementPDI::GetElements(uint8 DepthPriorityGroup) const
{
	return DepthPriorityGroup ? ViewInfo->TopBatchedViewElements : ViewInfo->BatchedViewElements;
}

inline void FViewElementPDI::DrawSprite(
	const FVector& Position,
	float SizeX,
	float SizeY,
	const FTexture* Sprite,
	const FLinearColor& Color,
	uint8 DepthPriorityGroup,
	float U,
	float UL,
	float V,
	float VL,
	uint8 BlendMode
	)
{
	FBatchedElements &Elements = GetElements(DepthPriorityGroup);

	Elements.AddSprite(
		Position,
		SizeX,
		SizeY,
		Sprite,
		Color,
		CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId(),
		U,UL,V,VL,
		BlendMode
	);
}

inline void FViewElementPDI::AddReserveLines(uint8 DepthPriorityGroup, int32 NumLines, bool bDepthBiased)
{
	FBatchedElements& Elements = GetElements(DepthPriorityGroup);

	Elements.AddReserveLines( NumLines, bDepthBiased );
}

inline void FViewElementPDI::DrawLine(
	const FVector& Start,
	const FVector& End,
	const FLinearColor& Color,
	uint8 DepthPriorityGroup,
	float Thickness,
	float DepthBias,
	bool bScreenSpace
	)
{
	FBatchedElements &Elements = GetElements(DepthPriorityGroup);

	Elements.AddLine(
		Start,
		End,
		Color,
		CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId(),
		Thickness,
		DepthBias,
		bScreenSpace
	);
}

inline void FViewElementPDI::DrawPoint(
	const FVector& Position,
	const FLinearColor& Color,
	float PointSize,
	uint8 DepthPriorityGroup
	)
{
	float ScaledPointSize = PointSize;

	bool bIsPerspective = (ViewInfo->ViewMatrices.ProjMatrix.M[3][3] < 1.0f) ? true : false;
	if( !bIsPerspective )
	{
		const float ZoomFactor = FMath::Min<float>(View->ViewMatrices.ProjMatrix.M[0][0], View->ViewMatrices.ProjMatrix.M[1][1]);
		ScaledPointSize = ScaledPointSize / ZoomFactor;
	}

	FBatchedElements &Elements = GetElements(DepthPriorityGroup);

	Elements.AddPoint(
		Position,
		ScaledPointSize,
		Color,
		CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId()
	);
}

inline bool MeshBatchHasPrimitives(const FMeshBatch& Mesh)
{
	bool bHasPrimitives = true;
	const int32 NumElements = Mesh.Elements.Num();
	for (int32 ElementIndex = 0; ElementIndex < NumElements; ++ElementIndex)
	{
		const FMeshBatchElement& Element = Mesh.Elements[ElementIndex];
		bHasPrimitives = bHasPrimitives && Element.NumPrimitives > 0 && Element.NumInstances > 0;
	}
	return bHasPrimitives;
}

inline int32 FViewElementPDI::DrawMesh(const FMeshBatch& Mesh)
{
	if (ensure(MeshBatchHasPrimitives(Mesh)))
	{
		// Keep track of view mesh elements whether that have translucency.
		ViewInfo->bHasTranslucentViewMeshElements |= true;//Mesh.IsTranslucent() ? 1 : 0;

		uint8 DPGIndex = Mesh.DepthPriorityGroup;
		// Get the correct element list based on dpg index
		// Translucent view mesh elements in the foreground dpg are not supported yet
		TIndirectArray<FMeshBatch>& ViewMeshElementList = ( ( DPGIndex == SDPG_Foreground  ) ? ViewInfo->TopViewMeshElements : ViewInfo->ViewMeshElements );

		FMeshBatch* NewMesh = new(ViewMeshElementList) FMeshBatch(Mesh);
		NewMesh->BatchHitProxyId = CurrentHitProxy ? CurrentHitProxy->Id : FHitProxyId();

		return 1;
	}
	return 0;
}

#endif
