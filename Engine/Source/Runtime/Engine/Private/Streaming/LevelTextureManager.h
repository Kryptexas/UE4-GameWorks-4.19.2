// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
TextureInstanceManager.h: Definitions of classes used for texture streaming.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "StaticTextureInstanceManager.h"
#include "DynamicTextureInstanceManager.h"
#include "Templates/RefCounting.h"
#include "ContentStreaming.h"
#include "Engine/TextureStreamingTypes.h"
#include "Streaming/TextureStreamingHelpers.h"
#include "UObject/UObjectHash.h"

// The streaming data of a level.
class FLevelTextureManager
{
public:

	FLevelTextureManager(ULevel* InLevel, TextureInstanceTask::FDoWorkTask& AsyncTask) : Level(InLevel), bIsInitialized(false), StaticInstances(AsyncTask),  BuildStep(EStaticBuildStep::BuildTextureLookUpMap) {}

	ULevel* GetLevel() const { return Level; }

	FORCEINLINE bool HasTextureReferences() const { return StaticInstances.HasTextureReferences(); }

	// Remove the whole level. Optional list of textures referenced
	void Remove(FRemovedTextureArray* RemovedTextures);

	// Invalidate a component reference.

	void RemoveActorReferences(const AActor* Actor)
	{
		UnprocessedStaticActors.RemoveSingleSwap(Actor); 
	}

	void RemoveComponentReferences(const UPrimitiveComponent* Component, FRemovedTextureArray& RemovedTextures) 
	{ 
		// Check everywhere as the mobility can change in game.
		StaticInstances.Remove(Component, &RemovedTextures); 
		UnprocessedComponents.RemoveSingleSwap(Component); 
		PendingInsertionStaticPrimitives.RemoveSingleSwap(Component); 
	}

	const FStaticTextureInstanceManager& GetStaticInstances() const { return StaticInstances; }

	float GetWorldTime() const;

	FORCEINLINE FTextureInstanceAsyncView GetAsyncView() { return FTextureInstanceAsyncView(StaticInstances.GetAsyncView(true)); }
	FORCEINLINE const FTextureInstanceView* GetRawAsyncView() { return StaticInstances.GetAsyncView(false); }

	void IncrementalUpdate(FDynamicTextureInstanceManager& DynamicManager, FRemovedTextureArray& RemovedTextures, int64& NumStepsLeftForIncrementalBuild, float Percentage, bool bUseDynamicStreaming);

	uint32 GetAllocatedSize() const;

	bool IsInitialized() const { return bIsInitialized; }

	void NotifyLevelOffset(const FVector& Offset);

private:

	ULevel* Level;

	bool bIsInitialized;

	FStaticTextureInstanceManager StaticInstances;

	/** Incremental build implementation. */

	enum class EStaticBuildStep : uint8
	{
		BuildTextureLookUpMap,
		GetActors,
		GetComponents,
		ProcessComponents,
		NormalizeLightmapTexelFactors,
		CompileElements,
		WaitForRegistration,
		Done,
	};

	// The current step of the incremental build.
	EStaticBuildStep BuildStep;
	// The actors / components left to be processed in ProcessComponents
	TArray<const AActor*> UnprocessedStaticActors;
	TArray<const UPrimitiveComponent*> UnprocessedComponents;
	// The components that could not be processed by the incremental build.
	TArray<const UPrimitiveComponent*> PendingInsertionStaticPrimitives;
	// Reversed lookup for ULevel::StreamingTextureGuids.
	TMap<FGuid, int32> TextureGuidToLevelIndex;

	bool NeedsIncrementalBuild(int32 NumStepsLeftForIncrementalBuild) const;
	void IncrementalBuild(FDynamicTextureInstanceManager& DynamicComponentManager, FStreamingTextureLevelContext& LevelContext, bool bForceCompletion, int64& NumStepsLeft);
	
	// Add this primitive to either the static instance or the dynamic ones. Returns false if the primitive couldn't be handled correctly.
	bool AddPrimitive(FDynamicTextureInstanceManager& DynamicComponentManager, FStreamingTextureLevelContext& LevelContext, const UPrimitiveComponent* Primitive, bool bLevelIsVisible, float MaxTextureUVDensity);
};
