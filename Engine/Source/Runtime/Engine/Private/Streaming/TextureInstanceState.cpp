// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TextureInstanceState.cpp: Implementation of content streaming classes.
=============================================================================*/

#include "Streaming/TextureInstanceState.h"
#include "Components/PrimitiveComponent.h"
#include "Streaming/TextureInstanceView.inl"
#include "Engine/World.h"
#include "Engine/TextureStreamingTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/UObjectHash.h"
#include "RefCounting.h"

int32 FTextureInstanceState::AddBounds(const UPrimitiveComponent* Component)
{
	return AddBounds(Component->Bounds, PackedRelativeBox_Identity, Component, Component->LastRenderTimeOnScreen, Component->Bounds.Origin, 0, 0, FLT_MAX);
}

int32 FTextureInstanceState::AddBounds(const FBoxSphereBounds& Bounds, uint32 PackedRelativeBox, const UPrimitiveComponent* InComponent, float LastRenderTime, const FVector4& RangeOrigin, float MinDistance, float MinRange, float MaxRange)
{
	check(InComponent);

	int BoundsIndex = INDEX_NONE;

	while (!Bounds4Components.IsValidIndex(BoundsIndex) && FreeBoundIndices.Num() > 0)
	{
		BoundsIndex = FreeBoundIndices.Pop();
	}

	if (!Bounds4Components.IsValidIndex(BoundsIndex))
	{
		BoundsIndex = Bounds4.Num() * 4;
		Bounds4.Push(FBounds4());

		Bounds4Components.Push(nullptr);
		Bounds4Components.Push(nullptr);
		Bounds4Components.Push(nullptr);
		Bounds4Components.Push(nullptr);

		// Since each element contains 4 entries, add the 3 unused ones
		FreeBoundIndices.Push(BoundsIndex + 3);
		FreeBoundIndices.Push(BoundsIndex + 2);
		FreeBoundIndices.Push(BoundsIndex + 1);
	}

	Bounds4[BoundsIndex / 4].Set(BoundsIndex % 4, Bounds, PackedRelativeBox, LastRenderTime, RangeOrigin, MinDistance, MinRange, MaxRange);
	Bounds4Components[BoundsIndex] = InComponent;

	return BoundsIndex;
}

void FTextureInstanceState::RemoveBounds(int32 BoundsIndex)
{
	checkSlow(!FreeBoundIndices.Contains(BoundsIndex));

	// Because components can be removed in CheckRegistrationAndUnpackBounds, which iterates on BoundsToUnpack,
	// here we invalidate the index, instead of removing it, to avoid resizing the array.
	int32 BoundsToUnpackIndex = INDEX_NONE;
	if (BoundsToUnpack.Find(BoundsIndex, BoundsToUnpackIndex))
	{
		BoundsToUnpack[BoundsToUnpackIndex] = INDEX_NONE;
	}

	// If the BoundsIndex is out of range, the next code will crash.	
	if (!ensure(Bounds4Components.IsValidIndex(BoundsIndex)))
	{
		return;
	}

	// If note all indices were freed
	if (1 + FreeBoundIndices.Num() != Bounds4.Num() * 4)
	{
		FreeBoundIndices.Push(BoundsIndex);
		Bounds4[BoundsIndex / 4].Clear(BoundsIndex % 4);
		Bounds4Components[BoundsIndex] = nullptr;
	}
	else
	{
		Bounds4.Empty();
		Bounds4Components.Empty();
		FreeBoundIndices.Empty();
	}
}

void FTextureInstanceState::AddElement(const UPrimitiveComponent* InComponent, const UTexture2D* InTexture, int InBoundsIndex, float InTexelFactor, bool InForceLoad, int32*& ComponentLink)
{
	check(InComponent && InTexture);

	// Keep Max texel factor up to date.
	MaxTexelFactor = FMath::Max(InTexelFactor, MaxTexelFactor);

	int32 ElementIndex = INDEX_NONE;
	if (FreeElementIndices.Num())
	{
		ElementIndex = FreeElementIndices.Pop();
	}
	else
	{
		ElementIndex = Elements.Num();
		Elements.Push(FElement());
	}

	FElement& Element = Elements[ElementIndex];

	Element.Component = InComponent;
	Element.Texture = InTexture;
	Element.BoundsIndex = InBoundsIndex;
	Element.TexelFactor = InTexelFactor;
	Element.bForceLoad = InForceLoad;

	FTextureDesc* TextureDesc = TextureMap.Find(InTexture);
	if (TextureDesc)
	{
		FElement& TextureLinkElement = Elements[TextureDesc->HeadLink];

		// The new inserted element as the head element.
		Element.NextTextureLink = TextureDesc->HeadLink;
		TextureLinkElement.PrevTextureLink = ElementIndex;
		TextureDesc->HeadLink = ElementIndex;
	}
	else
	{
		TextureMap.Add(InTexture, FTextureDesc(ElementIndex, InTexture->LODGroup));
	}

	// Simple sanity check to ensure that the component link passed in param is the right one
	checkSlow(ComponentLink == ComponentMap.Find(InComponent));
	if (ComponentLink)
	{
		// The new inserted element as the head element.
		Element.NextComponentLink = *ComponentLink;
		*ComponentLink = ElementIndex;
	}
	else
	{
		ComponentLink = &ComponentMap.Add(InComponent, ElementIndex);
	}

	// Keep the compiled elements up to date if it was built.
	// This will happen when not all components could be inserted in the incremental build.
	if (HasCompiledElements()) 
	{
		CompiledTextureMap.FindOrAdd(Element.Texture).Add(FCompiledElement(Element));
	}
}

void FTextureInstanceState::RemoveElement(int32 ElementIndex, int32& NextComponentLink, int32& BoundsIndex, const UTexture2D*& Texture)
{
	FElement& Element = Elements[ElementIndex];
	NextComponentLink = Element.NextComponentLink; 
	BoundsIndex = Element.BoundsIndex; 

	// Removed compiled elements. This happens when a static component is not registered after the level became visible.
	if (HasCompiledElements())
	{
		CompiledTextureMap.FindChecked(Element.Texture).RemoveSingleSwap(FCompiledElement(Element), false);
	}

	// Unlink textures
	if (Element.Texture)
	{
		if (Element.PrevTextureLink == INDEX_NONE) // If NONE, that means this is the head of the texture list.
		{
			if (Element.NextTextureLink != INDEX_NONE) // Check if there are other entries for this texture.
			{
				 // Replace the head
				TextureMap.Find(Element.Texture)->HeadLink = Element.NextTextureLink;
				Elements[Element.NextTextureLink].PrevTextureLink = INDEX_NONE;
			}
			else // Otherwise, remove the texture entry
			{
				TextureMap.Remove(Element.Texture);
				CompiledTextureMap.Remove(Element.Texture);
				Texture = Element.Texture;
			}
		}
		else // Otherwise, just relink entries.
		{
			Elements[Element.PrevTextureLink].NextTextureLink = Element.NextTextureLink;

			if (Element.NextTextureLink != INDEX_NONE)
			{
				Elements[Element.NextTextureLink].PrevTextureLink = Element.PrevTextureLink;
			}
		}
	}

	// Clear the element and insert in free list.
	if (1 + FreeElementIndices.Num() != Elements.Num())
	{
		FreeElementIndices.Push(ElementIndex);
		Element = FElement();
	}
	else
	{
		Elements.Empty();
		FreeElementIndices.Empty();
	}
}

FORCEINLINE bool operator<(const FBoxSphereBounds& Lhs, const FBoxSphereBounds& Rhs)
{
	// Check that all bites of the structure are used!
	check(sizeof(FBoxSphereBounds) == sizeof(FBoxSphereBounds::Origin) + sizeof(FBoxSphereBounds::BoxExtent) + sizeof(FBoxSphereBounds::SphereRadius));

	return FMemory::Memcmp(&Lhs, &Rhs, sizeof(FBoxSphereBounds)) < 0;
}

void FTextureInstanceState::AddTextureElements(const UPrimitiveComponent* Component, const TArrayView<FStreamingTexturePrimitiveInfo>& TextureInstanceInfos, int32 BoundsIndex, int32*& ComponentLink)
{
	// Loop for each texture - texel factor group (a group being of same texel factor sign)
	for (int32 InfoIndex = 0; InfoIndex < TextureInstanceInfos.Num();)
	{
		const FStreamingTexturePrimitiveInfo& Info = TextureInstanceInfos[InfoIndex];
		float MergedTexelFactor = Info.TexelFactor;
		int32 NumOfMergedElements = 1;

		// Merge all texel factor >= 0 for the same texture, or all those < 0
		if (Info.TexelFactor >= 0)
		{
			for (int32 NextInfoIndex = InfoIndex + 1; NextInfoIndex < TextureInstanceInfos.Num(); ++NextInfoIndex)
			{
				const FStreamingTexturePrimitiveInfo& NextInfo = TextureInstanceInfos[NextInfoIndex];
				if (NextInfo.Texture == Info.Texture && NextInfo.TexelFactor >= 0)
				{
					MergedTexelFactor = FMath::Max(MergedTexelFactor, NextInfo.TexelFactor);
					++NumOfMergedElements;
				}
				else
				{
					break;
				}
			}
		}
		else // Info.TexelFactor < 0
		{
			for (int32 NextInfoIndex = InfoIndex + 1; NextInfoIndex < TextureInstanceInfos.Num(); ++NextInfoIndex)
			{
				const FStreamingTexturePrimitiveInfo& NextInfo = TextureInstanceInfos[NextInfoIndex];
				if (NextInfo.Texture == Info.Texture && NextInfo.TexelFactor < 0)
				{
					MergedTexelFactor = FMath::Min(MergedTexelFactor, NextInfo.TexelFactor);
					++NumOfMergedElements;
				}
				else
				{
					break;
				}
			}
		}
		AddElement(Component, Info.Texture, BoundsIndex, MergedTexelFactor, Component->bForceMipStreaming, ComponentLink);

		InfoIndex += NumOfMergedElements;
	}
}

EAddComponentResult FTextureInstanceState::AddComponent(const UPrimitiveComponent* Component, FStreamingTextureLevelContext& LevelContext, float MaxAllowedUIDensity)
{
	TArray<FStreamingTexturePrimitiveInfo> TextureInstanceInfos;
	Component->GetStreamingTextureInfoWithNULLRemoval(LevelContext, TextureInstanceInfos);
	// Texture entries are guarantied to be relevant here, except for bounds if the component is not registered.

	if (!TextureInstanceInfos.Num())
	{
		return EAddComponentResult::Fail;
	}

	// First check if all entries are below the max allowed UI density, otherwise abort immediately.
	if (MaxAllowedUIDensity > 0)
	{
		for (const FStreamingTexturePrimitiveInfo& Info : TextureInstanceInfos)
		{
			if (Info.TexelFactor > MaxAllowedUIDensity)
			{
				return EAddComponentResult::Fail_UIDensityConstraint;
			}
		}
	}

	if (!Component->IsRegistered())
	{
		// When the components are not registered, the bound will be generated from PackedRelativeBox in CheckRegistrationAndUnpackBounds.
		// Otherwise, the entry is not usable as we don't know the bound to use. The component will need to be reinstered later, once registered.
		// it will not be possible to recreate the bounds correctly.
		for (const FStreamingTexturePrimitiveInfo& Info : TextureInstanceInfos)
		{
			if (!Info.PackedRelativeBox)
			{
				return EAddComponentResult::Fail;
			}
		}

		// Sort by PackedRelativeBox, to identical bounds identical entries together
		// Sort by Texture to merge duplicate texture entries.
		// Then sort by TexelFactor, to merge negative entries together.
		TextureInstanceInfos.Sort([](const FStreamingTexturePrimitiveInfo& Lhs, const FStreamingTexturePrimitiveInfo& Rhs) 
		{
			if (Lhs.PackedRelativeBox == Rhs.PackedRelativeBox)
			{
				if (Lhs.Texture == Rhs.Texture)
				{
					return Lhs.TexelFactor < Rhs.TexelFactor;
				}
				return Lhs.Texture < Rhs.Texture;
			}
			return Lhs.PackedRelativeBox < Rhs.PackedRelativeBox;
		});

		int32* ComponentLink = ComponentMap.Find(Component);

		// Loop for each bound.
		for (int32 InfoIndex = 0; InfoIndex < TextureInstanceInfos.Num();)
		{
			const FStreamingTexturePrimitiveInfo& Info = TextureInstanceInfos[InfoIndex];

			int32 NumOfBoundReferences = 1;
			for (int32 NextInfoIndex = InfoIndex + 1; NextInfoIndex < TextureInstanceInfos.Num() && TextureInstanceInfos[NextInfoIndex].PackedRelativeBox == Info.PackedRelativeBox; ++NextInfoIndex)
			{
				++NumOfBoundReferences;
			}

			const int32 BoundsIndex = AddBounds(FBoxSphereBounds(ForceInit), Info.PackedRelativeBox, Component, Component->LastRenderTimeOnScreen, FVector(ForceInit), 0, 0, FLT_MAX);
			AddTextureElements(Component, TArrayView<FStreamingTexturePrimitiveInfo>(TextureInstanceInfos.GetData() + InfoIndex, NumOfBoundReferences), BoundsIndex, ComponentLink);
			BoundsToUnpack.Push(BoundsIndex);

			InfoIndex += NumOfBoundReferences;
		}
	}
	else
	{
		// Sort by Bounds, to merge identical bounds entries together
		// Sort by Texture to merge duplicate texture entries.
		// Then sort by TexelFactor, to merge negative entries together.
		TextureInstanceInfos.Sort([](const FStreamingTexturePrimitiveInfo& Lhs, const FStreamingTexturePrimitiveInfo& Rhs) 
		{
			if (Lhs.Bounds == Rhs.Bounds)
			{
				if (Lhs.Texture == Rhs.Texture)
				{
					return Lhs.TexelFactor < Rhs.TexelFactor;
				}
				return Lhs.Texture < Rhs.Texture;
			}
			return Lhs.Bounds < Rhs.Bounds;
		});

		int32* ComponentLink = ComponentMap.Find(Component);
		float MinDistance = 0, MinRange = 0, MaxRange = FLT_MAX;

		// Loop for each bound.
		for (int32 InfoIndex = 0; InfoIndex < TextureInstanceInfos.Num();)
		{
			const FStreamingTexturePrimitiveInfo& Info = TextureInstanceInfos[InfoIndex];

			int32 NumOfBoundReferences = 1;
			for (int32 NextInfoIndex = InfoIndex + 1; NextInfoIndex < TextureInstanceInfos.Num() && TextureInstanceInfos[NextInfoIndex].Bounds == Info.Bounds; ++NextInfoIndex)
			{
				++NumOfBoundReferences;
			}

			GetDistanceAndRange(Component, Info.Bounds, MinDistance, MinRange, MaxRange);
			const int32 BoundsIndex = AddBounds(Info.Bounds, PackedRelativeBox_Identity, Component, Component->LastRenderTimeOnScreen, Component->Bounds.Origin, MinDistance, MinRange, MaxRange);
			AddTextureElements(Component, TArrayView<FStreamingTexturePrimitiveInfo>(TextureInstanceInfos.GetData() + InfoIndex, NumOfBoundReferences), BoundsIndex, ComponentLink);

			InfoIndex += NumOfBoundReferences;
		}
	}
	return EAddComponentResult::Success;
}

EAddComponentResult FTextureInstanceState::AddComponentIgnoreBounds(const UPrimitiveComponent* Component, FStreamingTextureLevelContext& LevelContext)
{
	check(Component->IsRegistered()); // Must be registered otherwise bounds are invalid.

	TArray<FStreamingTexturePrimitiveInfo> TextureInstanceInfos;
	Component->GetStreamingTextureInfoWithNULLRemoval(LevelContext, TextureInstanceInfos);

	if (!TextureInstanceInfos.Num())
	{
		return EAddComponentResult::Fail;
	}

	// Sort by Texture to merge duplicate texture entries.
	// Then sort by TexelFactor, to merge negative entries together.
	TextureInstanceInfos.Sort([](const FStreamingTexturePrimitiveInfo& Lhs, const FStreamingTexturePrimitiveInfo& Rhs) 
	{
		if (Lhs.Texture == Rhs.Texture)
		{
			return Lhs.TexelFactor < Rhs.TexelFactor;
		}
		return Lhs.Texture < Rhs.Texture;
	});

	int32* ComponentLink = ComponentMap.Find(Component);
	AddTextureElements(Component, TextureInstanceInfos, AddBounds(Component), ComponentLink);
	return EAddComponentResult::Success;
}


void FTextureInstanceState::RemoveComponent(const UPrimitiveComponent* Component, FRemovedTextureArray* RemovedTextures)
{
	TArray<int32, TInlineAllocator<12> > RemovedBoundsIndices;
	int32 ElementIndex = INDEX_NONE;

	ComponentMap.RemoveAndCopyValue(Component, ElementIndex);
	while (ElementIndex != INDEX_NONE)
	{
		int32 BoundsIndex = INDEX_NONE;
		const UTexture2D* Texture = nullptr;

		RemoveElement(ElementIndex, ElementIndex, BoundsIndex, Texture);

		if (BoundsIndex != INDEX_NONE)
		{
			RemovedBoundsIndices.AddUnique(BoundsIndex);
		}

		if (Texture && RemovedTextures)
		{
			RemovedTextures->AddUnique(Texture);
		}
	};

	for (int32 I = 0; I < RemovedBoundsIndices.Num(); ++I)
	{
		RemoveBounds(RemovedBoundsIndices[I]);
	}
}

bool FTextureInstanceState::RemoveComponentReferences(const UPrimitiveComponent* Component) 
{ 
	// Because the async streaming task could be running, we can't change the async view state. 
	// We limit ourself to clearing the component ptr to avoid invalid access when updating visibility.

	int32* ComponentLink = ComponentMap.Find(Component);
	if (ComponentLink)
	{
		int32 ElementIndex = *ComponentLink;
		while (ElementIndex != INDEX_NONE)
		{
			FElement& Element = Elements[ElementIndex];
			if (Element.BoundsIndex != INDEX_NONE)
			{
				Bounds4Components[Element.BoundsIndex] = nullptr;
			}
			Element.Component = nullptr;

			ElementIndex = Element.NextComponentLink;
		}

		ComponentMap.Remove(Component);
		return true;
	}
	else
	{
		return false;
	}
}

void FTextureInstanceState::GetReferencedComponents(TArray<const UPrimitiveComponent*>& Components) const
{
	for (TMap<const UPrimitiveComponent*, int32>::TConstIterator It(ComponentMap); It; ++It)
	{
		Components.Add(It.Key());
	}
}

void FTextureInstanceState::UpdateBounds(const UPrimitiveComponent* Component)
{
	int32* ComponentLink = ComponentMap.Find(Component);
	if (ComponentLink)
	{
		int32 ElementIndex = *ComponentLink;
		while (ElementIndex != INDEX_NONE)
		{
			const FElement& Element = Elements[ElementIndex];
			if (Element.BoundsIndex != INDEX_NONE)
			{
				Bounds4[Element.BoundsIndex / 4].FullUpdate(Element.BoundsIndex % 4, Component->Bounds, Component->LastRenderTimeOnScreen);
			}
			ElementIndex = Element.NextComponentLink;
		}
	}
}

bool FTextureInstanceState::UpdateBounds(int32 BoundIndex)
{
	const UPrimitiveComponent* Component = ensure(Bounds4Components.IsValidIndex(BoundIndex)) ? Bounds4Components[BoundIndex] : nullptr;
	if (Component)
	{
		Bounds4[BoundIndex / 4].FullUpdate(BoundIndex % 4, Component->Bounds, Component->LastRenderTimeOnScreen);
		return true;
	}
	else
	{
		return false;
	}
}

bool FTextureInstanceState::ConditionalUpdateBounds(int32 BoundIndex)
{
	const UPrimitiveComponent* Component = ensure(Bounds4Components.IsValidIndex(BoundIndex)) ? Bounds4Components[BoundIndex] : nullptr;
	if (Component)
	{
		if (Component->Mobility != EComponentMobility::Static)
		{
			const FBoxSphereBounds Bounds = Component->Bounds;

			// Check if the bound is coherent as it could be updated while we read it (from async task).
			// We don't have to check the position, as if it was partially updated, this should be ok (interp)
			const float RadiusSquared = FMath::Square<float>(Bounds.SphereRadius);
			const float XSquared = FMath::Square<float>(Bounds.BoxExtent.X);
			const float YSquared = FMath::Square<float>(Bounds.BoxExtent.Y);
			const float ZSquared = FMath::Square<float>(Bounds.BoxExtent.Z);

			if (0.5f * FMath::Min3<float>(XSquared, YSquared, ZSquared) <= RadiusSquared && RadiusSquared <= 2.f * (XSquared + YSquared + ZSquared))
			{
				Bounds4[BoundIndex / 4].FullUpdate(BoundIndex % 4, Bounds, Component->LastRenderTimeOnScreen);
				return true;
			}
		}
		else // Otherwise we assume it is guarantied to be good.
		{
			Bounds4[BoundIndex / 4].FullUpdate(BoundIndex % 4, Component->Bounds, Component->LastRenderTimeOnScreen);
			return true;
		}
	}
	return false;
}


void FTextureInstanceState::UpdateLastRenderTime(int32 BoundIndex)
{
	const UPrimitiveComponent* Component = ensure(Bounds4Components.IsValidIndex(BoundIndex)) ? Bounds4Components[BoundIndex] : nullptr;
	if (Component)
	{
		Bounds4[BoundIndex / 4].UpdateLastRenderTime(BoundIndex % 4, Component->LastRenderTimeOnScreen);
	}
}

uint32 FTextureInstanceState::GetAllocatedSize() const
{
	int32 CompiledElementsSize = 0;
	for (TMap<const UTexture2D*, TArray<FCompiledElement> >::TConstIterator It(CompiledTextureMap); It; ++It)
	{
		CompiledElementsSize += It.Value().GetAllocatedSize();
	}

	return Bounds4.GetAllocatedSize() +
		Bounds4Components.GetAllocatedSize() +
		Elements.GetAllocatedSize() +
		FreeBoundIndices.GetAllocatedSize() +
		FreeElementIndices.GetAllocatedSize() +
		TextureMap.GetAllocatedSize() +
		CompiledTextureMap.GetAllocatedSize() + CompiledElementsSize +
		ComponentMap.GetAllocatedSize();
}

int32 FTextureInstanceState::CompileElements()
{
	CompiledTextureMap.Empty();
	MaxTexelFactor = 0;

	// First create an entry for all elements, so that there are no reallocs when inserting each compiled elements.
	for (TMap<const UTexture2D*, FTextureDesc>::TConstIterator TextureIt(TextureMap); TextureIt; ++TextureIt)
	{
		const UTexture2D* Texture = TextureIt.Key();
		CompiledTextureMap.Add(Texture);
	}

	// Then fill in each array.
	for (TMap<const UTexture2D*, TArray<FCompiledElement> >::TIterator It(CompiledTextureMap); It; ++It)
	{
		const UTexture2D* Texture = It.Key();
		TArray<FCompiledElement>& CompiledElemements = It.Value();

		int32 CompiledElementCount = 0;
		for (auto ElementIt = GetElementIterator(Texture); ElementIt; ++ElementIt)
		{
			++CompiledElementCount;
		}
		CompiledElemements.AddUninitialized(CompiledElementCount);

		CompiledElementCount = 0;
		for (auto ElementIt = GetElementIterator(Texture); ElementIt; ++ElementIt)
		{
			const float TexelFactor = ElementIt.GetTexelFactor();
			// No need to care about force load as MaxTexelFactor is used to ignore far away levels.
			MaxTexelFactor = FMath::Max(TexelFactor, MaxTexelFactor);

			CompiledElemements[CompiledElementCount].BoundsIndex = ElementIt.GetBoundsIndex();
			CompiledElemements[CompiledElementCount].TexelFactor = TexelFactor;
			CompiledElemements[CompiledElementCount].bForceLoad = ElementIt.GetForceLoad();;

			++CompiledElementCount;
		}
	}
	return CompiledTextureMap.Num();
}

int32 FTextureInstanceState::CheckRegistrationAndUnpackBounds(TArray<const UPrimitiveComponent*>& RemovedComponents)
{
	for (int32 BoundIndex : BoundsToUnpack)
	{
		if (Bounds4Components.IsValidIndex(BoundIndex))
		{
			const UPrimitiveComponent* Component = Bounds4Components[BoundIndex];
			if (Component)
			{
				// At this point the component must be registered. If the proxy is valid, reject any component without one.
				// This would be hidden primitives, and also editor / debug primitives.
				if (Component->IsRegistered() && (!Component->IsRenderStateCreated() || Component->SceneProxy))
				{
					Bounds4[BoundIndex / 4].UnpackBounds(BoundIndex % 4, Component);
				}
				else // Here we can remove the component, as the async task is not yet using this.
				{
					RemoveComponent(Component, nullptr);
					RemovedComponents.Add(Component);
				}
			}
		}
	}

	const int32 NumSteps = BoundsToUnpack.Num();
	BoundsToUnpack.Empty();
	return NumSteps;
}

bool FTextureInstanceState::MoveBound(int32 SrcBoundIndex, int32 DstBoundIndex)
{
	check(!HasCompiledElements() && !BoundsToUnpack.Num()); // Defrag is for the dynamic elements which does not support dynamic compiled elements.

	if (Bounds4Components.IsValidIndex(DstBoundIndex) && Bounds4Components.IsValidIndex(SrcBoundIndex) && !Bounds4Components[DstBoundIndex] && Bounds4Components[SrcBoundIndex])
	{
		int32 FreeListIndex = FreeBoundIndices.Find(DstBoundIndex);
		if (FreeListIndex == INDEX_NONE)
		{
			return false; // The destination is not free.
		}
		// Update the free list.
		FreeBoundIndices[FreeListIndex] = SrcBoundIndex;

		const UPrimitiveComponent* Component = Bounds4Components[SrcBoundIndex];

		// Update the elements.
		int32* ComponentLink = ComponentMap.Find(Component);
		if (ComponentLink)
		{
			int32 ElementIndex = *ComponentLink;
			while (ElementIndex != INDEX_NONE)
			{
				FElement& Element = Elements[ElementIndex];
				
				// Sanity check to ensure elements and bounds are still linked correctly!
				check(Element.Component == Component);

				if (Element.BoundsIndex == SrcBoundIndex)
				{
					Element.BoundsIndex = DstBoundIndex;
				}
				ElementIndex = Element.NextComponentLink;
			}
		}

		// Update the component ptrs.
		Bounds4Components[DstBoundIndex] = Component;
		Bounds4Components[SrcBoundIndex] = nullptr;

		UpdateBounds(DstBoundIndex); // Update the bounds using the component.
		Bounds4[SrcBoundIndex / 4].Clear(SrcBoundIndex % 4);	

		return true;
	}
	else
	{
		return false;
	}
}

void FTextureInstanceState::TrimBounds()
{
	const int32 DefragThreshold = 8; // Must be a multiple of 4
	check(NumBounds4() * 4 == NumBounds());

	bool bUpdateFreeBoundIndices = false;

	// Here we check the bound components from low indices to high indices
	// because there are more chance that the lower range indices fail.
	// (since the incremental update move null component to the end)
	bool bDefragRangeIsFree = true;
	while (bDefragRangeIsFree)
	{
		const int32 LowerBoundThreshold = NumBounds() - DefragThreshold;
		if (Bounds4Components.IsValidIndex(LowerBoundThreshold))
		{
			for (int BoundIndex = LowerBoundThreshold; BoundIndex < NumBounds(); ++BoundIndex)
			{
				if (Bounds4Components[BoundIndex])
				{
					bDefragRangeIsFree = false;
					break;
				}
				else
				{
					checkSlow(FreeBoundIndices.Contains(BoundIndex));
				}
			}

			if (bDefragRangeIsFree)
			{
				Bounds4.RemoveAt(Bounds4.Num() - DefragThreshold / 4, DefragThreshold / 4, false);
				Bounds4Components.RemoveAt(Bounds4Components.Num() - DefragThreshold, DefragThreshold, false);
				bUpdateFreeBoundIndices = true;
			}
		}
		else
		{
			bDefragRangeIsFree = false;
		}
	} 

	if (bUpdateFreeBoundIndices)
	{
		// The bounds are cleared outside the range loop to prevent parsing elements multiple times.
		for (int Index = 0; Index < FreeBoundIndices.Num(); ++Index)
		{
			if (FreeBoundIndices[Index] >= NumBounds())
			{
				FreeBoundIndices.RemoveAtSwap(Index);
				--Index;
			}
		}
		check(NumBounds4() * 4 == NumBounds());
	}
}

void FTextureInstanceState::OffsetBounds(const FVector& Offset)
{
	for (int32 BoundIndex = 0; BoundIndex < Bounds4Components.Num(); ++BoundIndex)
	{
		if (Bounds4Components[BoundIndex])
		{
			Bounds4[BoundIndex / 4].OffsetBounds(BoundIndex % 4, Offset);
		}
	}
}
