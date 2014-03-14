// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticMeshDrawList.inl: Static mesh draw list implementation.
=============================================================================*/

#ifndef __STATICMESHDRAWLIST_INL__
#define __STATICMESHDRAWLIST_INL__

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::FElementHandle::Remove()
{
	// Make a copy of this handle's variables on the stack, since the call to Elements.RemoveSwap deletes the handle.
	TStaticMeshDrawList* const LocalDrawList = StaticMeshDrawList;
	FDrawingPolicyLink* const LocalDrawingPolicyLink = &LocalDrawList->DrawingPolicySet(SetId);
	const int32 LocalElementIndex = ElementIndex;

	checkSlow(LocalDrawingPolicyLink->SetId == SetId);

	// Unlink the mesh from this draw list.
	LocalDrawingPolicyLink->Elements[ElementIndex].Mesh->UnlinkDrawList(this);
	LocalDrawingPolicyLink->Elements[ElementIndex].Mesh = NULL;

	checkSlow(LocalDrawingPolicyLink->Elements.Num() == LocalDrawingPolicyLink->CompactElements.Num());

	// Remove this element from the drawing policy's element list.
	const uint32 LastDrawingPolicySize = LocalDrawingPolicyLink->GetSizeBytes();

	LocalDrawingPolicyLink->Elements.RemoveAtSwap(LocalElementIndex);
	LocalDrawingPolicyLink->CompactElements.RemoveAtSwap(LocalElementIndex);
	
	const uint32 CurrentDrawingPolicySize = LocalDrawingPolicyLink->GetSizeBytes();
	const uint32 DrawingPolicySizeDiff = LastDrawingPolicySize - CurrentDrawingPolicySize;

	LocalDrawList->TotalBytesUsed -= DrawingPolicySizeDiff;


	if (LocalElementIndex < LocalDrawingPolicyLink->Elements.Num())
	{
		// Fixup the element that was moved into the hole created by the removed element.
		LocalDrawingPolicyLink->Elements[LocalElementIndex].Handle->ElementIndex = LocalElementIndex;
	}

	// If this was the last element for the drawing policy, remove the drawing policy from the draw list.
	if(!LocalDrawingPolicyLink->Elements.Num())
	{
		LocalDrawList->TotalBytesUsed -= LocalDrawingPolicyLink->GetSizeBytes();

		LocalDrawList->OrderedDrawingPolicies.RemoveSingle(LocalDrawingPolicyLink->SetId);
		LocalDrawList->DrawingPolicySet.Remove(LocalDrawingPolicyLink->SetId);
	}
}

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::DrawElement(
	const FViewInfo& View,
	const FElement& Element,
	uint64 BatchElementMask,
	FDrawingPolicyLink* DrawingPolicyLink,
	bool& bDrawnShared
	)
{
	FScopeCycleCounter Context(Element.Mesh->PrimitiveSceneInfo->Proxy->GetStatId());

	if (!bDrawnShared)
	{
		if (!IsValidRef(DrawingPolicyLink->BoundShaderState))
		{
			DrawingPolicyLink->CreateBoundShaderState();
		}
		DrawingPolicyLink->DrawingPolicy.DrawShared(&View,DrawingPolicyLink->BoundShaderState);
		bDrawnShared = true;
	}
	
	int32 BatchElementIndex = 0;
	do
	{
		if(BatchElementMask & 1)
		{
			for (int32 bBackFace = 0; bBackFace < (DrawingPolicyLink->DrawingPolicy.NeedsBackfacePass() ? 2 : 1); bBackFace++)
			{
				INC_DWORD_STAT(STAT_StaticDrawListMeshDrawCalls);

				DrawingPolicyLink->DrawingPolicy.SetMeshRenderState(
					View,
					Element.Mesh->PrimitiveSceneInfo->Proxy,
					*Element.Mesh,
					BatchElementIndex,
					!!bBackFace,
					Element.PolicyData
					);
				DrawingPolicyLink->DrawingPolicy.DrawMesh(*Element.Mesh,BatchElementIndex);
			}
		}

		BatchElementMask >>= 1;
		BatchElementIndex++;
	} while(BatchElementMask);
}

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::AddMesh(
	FStaticMesh* Mesh,
	const ElementPolicyDataType& PolicyData,
	const DrawingPolicyType& InDrawingPolicy
	)
{
	// Check for an existing drawing policy matching the mesh's drawing policy.
	FDrawingPolicyLink* DrawingPolicyLink = DrawingPolicySet.Find(InDrawingPolicy);
	if(!DrawingPolicyLink)
	{
		// If no existing drawing policy matches the mesh, create a new one.
		const FSetElementId DrawingPolicyLinkId = DrawingPolicySet.Add(FDrawingPolicyLink(this,InDrawingPolicy));

		DrawingPolicyLink = &DrawingPolicySet(DrawingPolicyLinkId);
		DrawingPolicyLink->SetId = DrawingPolicyLinkId;

		TotalBytesUsed += DrawingPolicyLink->GetSizeBytes();

		// Insert the drawing policy into the ordered drawing policy list.
		int32 MinIndex = 0;
		int32 MaxIndex = OrderedDrawingPolicies.Num() - 1;
		while(MinIndex < MaxIndex)
		{
			int32 PivotIndex = (MaxIndex + MinIndex) / 2;
			int32 CompareResult = CompareDrawingPolicy(DrawingPolicySet(OrderedDrawingPolicies[PivotIndex]).DrawingPolicy,DrawingPolicyLink->DrawingPolicy);
			if(CompareResult < 0)
			{
				MinIndex = PivotIndex + 1;
			}
			else if(CompareResult > 0)
			{
				MaxIndex = PivotIndex;
			}
			else
			{
				MinIndex = MaxIndex = PivotIndex;
			}
		};
		check(MinIndex >= MaxIndex);
		OrderedDrawingPolicies.Insert(DrawingPolicyLinkId,MinIndex);
	}

	const int32 ElementIndex = DrawingPolicyLink->Elements.Num();
	const SIZE_T PreviousElementsSize = DrawingPolicyLink->Elements.GetAllocatedSize();
	const SIZE_T PreviousCompactElementsSize = DrawingPolicyLink->CompactElements.GetAllocatedSize();
	FElement* Element = new(DrawingPolicyLink->Elements) FElement(Mesh, PolicyData, this, DrawingPolicyLink->SetId, ElementIndex);
	new(DrawingPolicyLink->CompactElements) FElementCompact(Mesh->Id);
	TotalBytesUsed += DrawingPolicyLink->Elements.GetAllocatedSize() - PreviousElementsSize + DrawingPolicyLink->CompactElements.GetAllocatedSize() - PreviousCompactElementsSize;
	Mesh->LinkDrawList(Element->Handle);
}

template<typename DrawingPolicyType>
TStaticMeshDrawList<DrawingPolicyType>::TStaticMeshDrawList()
{
	if(IsInRenderingThread())
	{
		InitResource();
	}
	else
	{
		BeginInitResource(this);
	}
}

template<typename DrawingPolicyType>
TStaticMeshDrawList<DrawingPolicyType>::~TStaticMeshDrawList()
{
	if(IsInRenderingThread())
	{
		ReleaseResource();
	}
	else
	{
		BeginReleaseResource(this);
	}

#if STATS
	for (typename TArray<FSetElementId>::TConstIterator PolicyIt(OrderedDrawingPolicies); PolicyIt; ++PolicyIt)
	{
		const FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet(*PolicyIt);
		TotalBytesUsed -= DrawingPolicyLink->GetSizeBytes();
	}
#endif
}

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::ReleaseRHI()
{
	for(typename TDrawingPolicySet::TIterator DrawingPolicyIt(DrawingPolicySet);DrawingPolicyIt;++DrawingPolicyIt)
	{
		DrawingPolicyIt->ReleaseBoundShaderState();
	}
}

template<typename DrawingPolicyType>
bool TStaticMeshDrawList<DrawingPolicyType>::DrawVisible(
	const FViewInfo& View,
	const TBitArray<SceneRenderingBitArrayAllocator>& StaticMeshVisibilityMap
	)
{
	bool bDirty = false;
	for(typename TArray<FSetElementId>::TConstIterator PolicyIt(OrderedDrawingPolicies); PolicyIt; ++PolicyIt)
	{
		FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet(*PolicyIt);
		bool bDrawnShared = false;
		FPlatformMisc::Prefetch(DrawingPolicyLink->CompactElements.GetTypedData());
		const int32 NumElements = DrawingPolicyLink->Elements.Num();
		FPlatformMisc::Prefetch(&DrawingPolicyLink->CompactElements.GetTypedData()->VisibilityBitReference);
		const FElementCompact* CompactElementPtr = DrawingPolicyLink->CompactElements.GetTypedData();
		for(int32 ElementIndex = 0; ElementIndex < NumElements; ElementIndex++, CompactElementPtr++)
		{
			if(StaticMeshVisibilityMap.AccessCorrespondingBit(CompactElementPtr->VisibilityBitReference))
			{
				const FElement& Element = DrawingPolicyLink->Elements[ElementIndex];
				INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,Element.Mesh->GetNumPrimitives());
				// Avoid the virtual call looking up batch visibility if there is only one element.
				uint32 BatchElementMask = Element.Mesh->Elements.Num() == 1 ? 1 : Element.Mesh->VertexFactory->GetStaticBatchElementVisibility(View,Element.Mesh);
				DrawElement(View, Element, BatchElementMask, DrawingPolicyLink, bDrawnShared);
				bDirty = true;
			}
		}
	}
	return bDirty;
}

template<typename DrawingPolicyType>
bool TStaticMeshDrawList<DrawingPolicyType>::DrawVisible(
	const FViewInfo& View,
	const TBitArray<SceneRenderingBitArrayAllocator>& StaticMeshVisibilityMap,
	const TArray<uint64,SceneRenderingAllocator>& BatchVisibilityArray
	)
{
	bool bDirty = false;
	for(typename TArray<FSetElementId>::TConstIterator PolicyIt(OrderedDrawingPolicies); PolicyIt; ++PolicyIt)
	{
		FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet(*PolicyIt);
		bool bDrawnShared = false;
		FPlatformMisc::Prefetch(DrawingPolicyLink->CompactElements.GetTypedData());
		const int32 NumElements = DrawingPolicyLink->Elements.Num();
		FPlatformMisc::Prefetch(&DrawingPolicyLink->CompactElements.GetTypedData()->VisibilityBitReference);
		const FElementCompact* CompactElementPtr = DrawingPolicyLink->CompactElements.GetTypedData();
		for(int32 ElementIndex = 0; ElementIndex < NumElements; ElementIndex++, CompactElementPtr++)
		{
			if(StaticMeshVisibilityMap.AccessCorrespondingBit(CompactElementPtr->VisibilityBitReference))
			{
				const FElement& Element = DrawingPolicyLink->Elements[ElementIndex];
				INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,Element.Mesh->GetNumPrimitives());
				// Avoid the cache miss looking up batch visibility if there is only one element.
				uint64 BatchElementMask = Element.Mesh->Elements.Num() == 1 ? 1 : BatchVisibilityArray[Element.Mesh->Id];
				DrawElement(View, Element, BatchElementMask, DrawingPolicyLink, bDrawnShared);
				bDirty = true;
			}
		}
	}
	return bDirty;
}

template<typename DrawingPolicyType>
int32 TStaticMeshDrawList<DrawingPolicyType>::DrawVisibleFrontToBack(
	const FViewInfo& View,
	const TBitArray<SceneRenderingBitArrayAllocator>& StaticMeshVisibilityMap,
	const TArray<uint64,SceneRenderingAllocator>& BatchVisibilityArray,
	int32 MaxToDraw
	)
{
	int32 NumDraws = 0;
	TArray<FDrawListSortKey,SceneRenderingAllocator> SortKeys;
	const FVector ViewLocation = View.ViewLocation;
	SortKeys.Reserve(64);

	for(typename TArray<FSetElementId>::TConstIterator PolicyIt(OrderedDrawingPolicies); PolicyIt; ++PolicyIt)
	{
		FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet(*PolicyIt);
		FVector DrawingPolicyCenter = DrawingPolicyLink->CachedBoundingSphere.Center;
		FPlatformMisc::Prefetch(DrawingPolicyLink->CompactElements.GetTypedData());
		const int32 NumElements = DrawingPolicyLink->Elements.Num();
		FPlatformMisc::Prefetch(&DrawingPolicyLink->CompactElements.GetTypedData()->VisibilityBitReference);
		const FElementCompact* CompactElementPtr = DrawingPolicyLink->CompactElements.GetTypedData();
		for(int32 ElementIndex = 0; ElementIndex < NumElements; ElementIndex++, CompactElementPtr++)
		{
			if(StaticMeshVisibilityMap.AccessCorrespondingBit(CompactElementPtr->VisibilityBitReference))
			{
				const FElement& Element = DrawingPolicyLink->Elements[ElementIndex];
				const FBoxSphereBounds& Bounds = Element.Bounds;
				float DistanceSq = (Bounds.Origin - ViewLocation).SizeSquared();
				float DrawingPolicyDistanceSq = (DrawingPolicyCenter - ViewLocation).SizeSquared();
				SortKeys.Add(GetSortKey(Element.bBackground,Bounds.SphereRadius,DrawingPolicyDistanceSq,PolicyIt->AsInteger(),DistanceSq,ElementIndex));
			}
		}
	}

	SortKeys.Sort();

	int32 LastDrawingPolicyIndex = INDEX_NONE;
	FDrawingPolicyLink* DrawingPolicyLink = NULL;
	bool bDrawnShared = false;
	for (int32 SortedIndex = 0, NumSorted = FMath::Min(SortKeys.Num(),MaxToDraw); SortedIndex < NumSorted; ++SortedIndex)
	{
		int32 DrawingPolicyIndex = SortKeys[SortedIndex].Fields.DrawingPolicyIndex;
		int32 ElementIndex = SortKeys[SortedIndex].Fields.MeshElementIndex;
		if (DrawingPolicyIndex != LastDrawingPolicyIndex)
		{
			DrawingPolicyLink = &DrawingPolicySet(FSetElementId::FromInteger(DrawingPolicyIndex));
			LastDrawingPolicyIndex = DrawingPolicyIndex;
			bDrawnShared = false;
		}

		const FElement& Element = DrawingPolicyLink->Elements[ElementIndex];
		INC_DWORD_STAT_BY(STAT_StaticMeshTriangles,Element.Mesh->GetNumPrimitives());
		// Avoid the cache miss looking up batch visibility if there is only one element.
		uint64 BatchElementMask = Element.Mesh->Elements.Num() == 1 ? 1 : BatchVisibilityArray[Element.Mesh->Id];
		DrawElement(View, Element, BatchElementMask, DrawingPolicyLink, bDrawnShared);
		NumDraws++;
	}

	return NumDraws;
}

template<typename DrawingPolicyType>
FVector TStaticMeshDrawList<DrawingPolicyType>::SortViewPosition = FVector(0);

template<typename DrawingPolicyType>
typename TStaticMeshDrawList<DrawingPolicyType>::TDrawingPolicySet* TStaticMeshDrawList<DrawingPolicyType>::SortDrawingPolicySet;

template<typename DrawingPolicyType>
int32 TStaticMeshDrawList<DrawingPolicyType>::Compare(FSetElementId A, FSetElementId B)
{
	const FSphere& BoundsA = (*SortDrawingPolicySet)(A).CachedBoundingSphere;
	const FSphere& BoundsB = (*SortDrawingPolicySet)(B).CachedBoundingSphere;

	// Assume state buckets with large bounds are background geometry
	if (BoundsA.W >= HALF_WORLD_MAX / 2 && BoundsB.W < HALF_WORLD_MAX / 2)
	{
		return 1;
	}
	else if (BoundsB.W >= HALF_WORLD_MAX / 2 && BoundsA.W < HALF_WORLD_MAX / 2)
	{
		return -1;
	}
	else
	{
		const float DistanceASquared = (BoundsA.Center - SortViewPosition).SizeSquared();
		const float DistanceBSquared = (BoundsB.Center - SortViewPosition).SizeSquared();
		// Sort front to back
		return DistanceASquared > DistanceBSquared ? 1 : -1;
	}
}

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::SortFrontToBack(FVector ViewPosition)
{
	// Cache policy link bounds
	for (typename TDrawingPolicySet::TIterator DrawingPolicyIt(DrawingPolicySet); DrawingPolicyIt; ++DrawingPolicyIt)
	{
		FBoxSphereBounds AccumulatedBounds(ForceInit);

		FDrawingPolicyLink& DrawingPolicyLink = *DrawingPolicyIt;
		for (int32 ElementIndex = 0; ElementIndex < DrawingPolicyLink.Elements.Num(); ElementIndex++)
		{
			FElement& Element = DrawingPolicyLink.Elements[ElementIndex];

			if (ElementIndex == 0)
			{
				AccumulatedBounds = Element.Bounds;
			}
			else
			{
				AccumulatedBounds = AccumulatedBounds + Element.Bounds;
			}
		}
		DrawingPolicyIt->CachedBoundingSphere = AccumulatedBounds.GetSphere();
	}

	SortViewPosition = ViewPosition;
	SortDrawingPolicySet = &DrawingPolicySet;

	OrderedDrawingPolicies.Sort( TCompareStaticMeshDrawList<DrawingPolicyType>() );
}

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::GetUsedPrimitivesBasedOnMaterials(const TArray<const FMaterial*>& Materials, TArray<FPrimitiveSceneInfo*>& PrimitivesToUpdate)
{
	for (typename TDrawingPolicySet::TIterator DrawingPolicyIt(DrawingPolicySet); DrawingPolicyIt; ++DrawingPolicyIt)
	{
		FDrawingPolicyLink& DrawingPolicyLink = *DrawingPolicyIt;

		for (int32 ElementIndex = 0; ElementIndex < DrawingPolicyLink.Elements.Num(); ElementIndex++)
		{
			FElement& Element = DrawingPolicyLink.Elements[ElementIndex];

			// Compare to the referenced material, not the material used for rendering
			// In the case of async shader compiling, MaterialRenderProxy->GetMaterial() is going to be the default material until the async compiling is complete
			const FMaterialRenderProxy* Proxy = Element.Mesh->MaterialRenderProxy;

			if (Proxy)
			{
				FMaterial* MaterialResource = Proxy->GetMaterialNoFallback(GRHIFeatureLevel);

				if (Materials.Contains(MaterialResource))
				{
					PrimitivesToUpdate.AddUnique(Element.Mesh->PrimitiveSceneInfo);
				}
			}
		}
	}
}

template<typename DrawingPolicyType>
int32 TStaticMeshDrawList<DrawingPolicyType>::NumMeshes() const
{
	int32 TotalMeshes=0;
	for(typename TArray<FSetElementId>::TConstIterator PolicyIt(OrderedDrawingPolicies); PolicyIt; ++PolicyIt)
	{
		const FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet(*PolicyIt);
		TotalMeshes += DrawingPolicyLink->Elements.Num();
	}
	return TotalMeshes;
}

template<typename DrawingPolicyType>
FDrawListStats TStaticMeshDrawList<DrawingPolicyType>::GetStats() const
{
	FDrawListStats Stats = {0};
	TArray<int32> MeshCounts;
	for(typename TArray<FSetElementId>::TConstIterator PolicyIt(OrderedDrawingPolicies); PolicyIt; ++PolicyIt)
	{
		const FDrawingPolicyLink* DrawingPolicyLink = &DrawingPolicySet(*PolicyIt);
		int32 NumMeshes = DrawingPolicyLink->Elements.Num();
		Stats.NumDrawingPolicies++;
		Stats.NumMeshes += NumMeshes;
		MeshCounts.Add(NumMeshes);
	}
	if (MeshCounts.Num())
	{
		MeshCounts.Sort();
		Stats.MedianMeshesPerDrawingPolicy = MeshCounts[MeshCounts.Num() / 2];
		Stats.MaxMeshesPerDrawingPolicy = MeshCounts.Last();
		while (Stats.NumSingleMeshDrawingPolicies < MeshCounts.Num()
			&& MeshCounts[Stats.NumSingleMeshDrawingPolicies] == 1)
		{
			Stats.NumSingleMeshDrawingPolicies++;
		}
	}
	return Stats;
}

template<typename DrawingPolicyType>
void TStaticMeshDrawList<DrawingPolicyType>::ApplyWorldOffset(FVector InOffset)
{
	for (auto It = DrawingPolicySet.CreateIterator(); It; ++It)
	{
		FDrawingPolicyLink& DrawingPolicyLink = (*It);
		for (int32 ElementIndex = 0; ElementIndex < DrawingPolicyLink.Elements.Num(); ElementIndex++)
		{
			FElement& Element = DrawingPolicyLink.Elements[ElementIndex];
			Element.Bounds.Origin+= InOffset;
		}

		DrawingPolicyLink.CachedBoundingSphere.Center+= InOffset;
	}
}

#endif
