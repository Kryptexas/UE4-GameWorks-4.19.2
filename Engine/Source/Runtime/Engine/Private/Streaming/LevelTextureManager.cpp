// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LevelTextureManager.cpp: Implementation of content streaming classes.
=============================================================================*/

#include "Streaming/LevelTextureManager.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

void FLevelTextureManager::Remove(FDynamicTextureInstanceManager& DynamicComponentManager, FRemovedTextureArray& RemovedTextures)
{ 
	// Mark all static textures for removal.
	for (auto It = StaticInstances.GetTextureIterator(); It; ++It)
	{
		RemovedTextures.Push(*It);
	}

	// Mark all dynamic textures for removal.
	for (const UPrimitiveComponent* Component : DynamicComponents)
	{
		DynamicComponentManager.Remove(Component, RemovedTextures);
	}	
	DynamicComponents.Empty();

	BuildStep = EStaticBuildStep::BuildTextureLookUpMap;
	StaticActorsWithNonStaticPrimitives.Empty();
	UnprocessedStaticActors.Empty();
	UnprocessedStaticComponents.Empty();
	PendingInsertionStaticPrimitives.Empty();
	TextureGuidToLevelIndex.Empty();
	bIsInitialized = false;
}

float FLevelTextureManager::GetWorldTime() const
{
	if (!GIsEditor && Level)
	{
		UWorld* World = Level->GetWorld();

		// When paused, updating the world time sometimes break visibility logic.
		if (World && !World->IsPaused())
		{
			return World->GetTimeSeconds();
		}
	}
	return 0;
}

void FLevelTextureManager::IncrementalBuild(FStreamingTextureLevelContext& LevelContext, bool bForceCompletion, int64& NumStepsLeft)
{
	check(Level);

	switch (BuildStep)
	{
	case EStaticBuildStep::BuildTextureLookUpMap:
	{
		// Build the map to convert from a guid to the level index. 
		TextureGuidToLevelIndex.Reserve(Level->StreamingTextureGuids.Num());
		for (int32 TextureIndex = 0; TextureIndex < Level->StreamingTextureGuids.Num(); ++TextureIndex)
		{
			TextureGuidToLevelIndex.Add(Level->StreamingTextureGuids[TextureIndex], TextureIndex);
		}
		NumStepsLeft -= Level->StreamingTextureGuids.Num();
		BuildStep = EStaticBuildStep::GetActors;

		// Update the level context with the texture guid map. This is required in case the incremental build runs more steps.
		LevelContext = FStreamingTextureLevelContext(EMaterialQualityLevel::Num, Level, &TextureGuidToLevelIndex);
		break;
	}
	case EStaticBuildStep::GetActors:
	{
		// This should be unset at first.
		check(UnprocessedStaticActors.Num() == 0 && UnprocessedStaticComponents.Num() == 0 && PendingInsertionStaticPrimitives.Num() == 0);

		UnprocessedStaticActors.Empty(Level->Actors.Num());
		for (const AActor* Actor : Level->Actors)
		{
			if (Actor && Actor->IsRootComponentStatic())
			{
				UnprocessedStaticActors.Push(Actor);
			}
		}

		NumStepsLeft -= (int64)FMath::Max<int32>(Level->Actors.Num() / 16, 1); // div 16 because this is light weight.
		BuildStep = EStaticBuildStep::GetComponents;
		break;
	}
	case EStaticBuildStep::GetComponents:
	{
		while ((bForceCompletion || NumStepsLeft > 0) && UnprocessedStaticActors.Num())
		{
			const AActor* StaticActor = UnprocessedStaticActors.Pop(false);
			check(StaticActor);

			// Check if the actor is still static, as the mobility is allowed to change.
			if (StaticActor->IsRootComponentStatic())
			{
				TInlineComponentArray<UPrimitiveComponent*> Primitives;
				StaticActor->GetComponents<UPrimitiveComponent>(Primitives);

				bool bHasNonStaticPrimitives = false;
				for (UPrimitiveComponent* Primitive : Primitives)
				{
					check(Primitive);
					if (Primitive->Mobility == EComponentMobility::Static)
					{
						UnprocessedStaticComponents.Push(Primitive);
					}
					else
					{
						bHasNonStaticPrimitives = true;
					}
				}

				// Mark this actor to process non static component later. (after visibility)
				if (bHasNonStaticPrimitives)
				{	
					StaticActorsWithNonStaticPrimitives.Push(StaticActor);
				}

				NumStepsLeft -= (int64)FMath::Max<int32>(Primitives.Num() / 16, 1); // div 16 because this is light weight.
			}
		}

		if (!UnprocessedStaticActors.Num())
		{
			UnprocessedStaticActors.Empty(); // Free the memory.
			BuildStep = EStaticBuildStep::ProcessComponents;
		}
		break;
	}
	case EStaticBuildStep::ProcessComponents:
	{
		while ((bForceCompletion || NumStepsLeft > 0) && UnprocessedStaticComponents.Num())
		{
			const UPrimitiveComponent* Primitive = UnprocessedStaticComponents.Pop(false);
			check(Primitive);

			if (Primitive->Mobility == EComponentMobility::Static)
			{
				// Try to insert the component, this will fail if some texture entry has not PackedRelativeBounds.
				if (!StaticInstances.Add(Primitive, LevelContext))
				{
					PendingInsertionStaticPrimitives.Add(Primitive);
				}
			}
			else // Otherwise, check if the root component is still static, to ensure the component gets processed.
			{
				const AActor* Owner = Primitive->GetOwner();
				if (Owner && Owner->IsRootComponentStatic())
				{
					StaticActorsWithNonStaticPrimitives.AddUnique(Owner);
				}
			}
			--NumStepsLeft;
		}

		if (!UnprocessedStaticComponents.Num())
		{
			UnprocessedStaticComponents.Empty(); // Free the memory.
			BuildStep = EStaticBuildStep::NormalizeLightmapTexelFactors;
		}
		break;
	}
	case EStaticBuildStep::NormalizeLightmapTexelFactors:
		// Unfortunately, PendingInsertionStaticPrimtivComponents won't be taken into account here.
		StaticInstances.NormalizeLightmapTexelFactor();
		BuildStep = EStaticBuildStep::CompileElements;
		break;
	case EStaticBuildStep::CompileElements:
		// Compile elements (to optimize runtime) for what is there.
		// PendingInsertionStaticPrimitives will be added after.
		NumStepsLeft -= (int64)StaticInstances.CompileElements();
		BuildStep = EStaticBuildStep::WaitForRegistration;
		break;
	case EStaticBuildStep::WaitForRegistration:
		if (Level->bIsVisible)
		{
			// Remove unregistered component and resolve the bounds using the packed relative boxes.
			NumStepsLeft -= (int64)StaticInstances.CheckRegistrationAndUnpackBounds();

			NumStepsLeft -= (int64)PendingInsertionStaticPrimitives.Num();

			// Insert the component we could not preprocess.
			while (PendingInsertionStaticPrimitives.Num())
			{
				const UPrimitiveComponent* Primitive = PendingInsertionStaticPrimitives.Pop(false);
				if (Primitive && Primitive->IsRegistered() && Primitive->SceneProxy)
				{
					StaticInstances.Add(Primitive, LevelContext);
				}
			}
			PendingInsertionStaticPrimitives.Empty(); // Free the memory.
			TextureGuidToLevelIndex.Empty();
			BuildStep = EStaticBuildStep::Done;
		}
		break;
	default:
		break;
	}
}

bool FLevelTextureManager::NeedsIncrementalBuild(int32 NumStepsLeftForIncrementalBuild) const
{
	check(Level);

	if (BuildStep == EStaticBuildStep::Done)
	{
		return false;
	}
	else if (Level->bIsVisible)
	{
		return true; // If visible, continue until done.
	}
	else // Otherwise, continue while there are incremental build steps available or we are waiting for visibility.
	{
		return BuildStep != EStaticBuildStep::WaitForRegistration && NumStepsLeftForIncrementalBuild > 0;
	}
}

void FLevelTextureManager::IncrementalUpdate(
	FDynamicTextureInstanceManager& DynamicComponentManager, 
	FRemovedTextureArray& RemovedTextures, 
	int64& NumStepsLeftForIncrementalBuild, 
	float Percentage, 
	bool bUseDynamicStreaming) 
{
	QUICK_SCOPE_CYCLE_COUNTER(FStaticComponentTextureManager_IncrementalUpdate);

	check(Level);

	if (NeedsIncrementalBuild(NumStepsLeftForIncrementalBuild))
	{
		FStreamingTextureLevelContext LevelContext(EMaterialQualityLevel::Num, Level, &TextureGuidToLevelIndex);
		do
		{
			IncrementalBuild(LevelContext, Level->bIsVisible, NumStepsLeftForIncrementalBuild);
		}
		while (NeedsIncrementalBuild(NumStepsLeftForIncrementalBuild));
	}

	if (BuildStep == EStaticBuildStep::Done)
	{
		if (Level->bIsVisible && !bIsInitialized)
		{
			FStreamingTextureLevelContext LevelContext(EMaterialQualityLevel::Num, Level);
			if (bUseDynamicStreaming)
			{
				TInlineComponentArray<UPrimitiveComponent*> Primitives;
		
				// Handle dynamic component for static actors
				for (int32 ActorIndex = 0; ActorIndex < StaticActorsWithNonStaticPrimitives.Num(); ++ActorIndex)
				{
					const AActor* Actor = StaticActorsWithNonStaticPrimitives[ActorIndex];
					check(Actor);

					// Check if the actor is still static as the mobility is allowed to change. If not, it will get processed in the next loop.
					bool AnyComponentAdded = false;
					if (Actor->IsRootComponentStatic())
					{
						Actor->GetComponents<UPrimitiveComponent>(Primitives);
						for (const UPrimitiveComponent* Primitive : Primitives)
						{
							check(Primitive);
							// Component with static mobility are already processed at this point.
							if (Primitive->IsRegistered() && Primitive->SceneProxy && Primitive->Mobility != EComponentMobility::Static)
							{
								DynamicComponentManager.Add(Primitive, LevelContext);
								DynamicComponents.Add(Primitive); // Track component added form this level so we can remove them later.
								AnyComponentAdded = true;
							}
						}
					}

					if (!AnyComponentAdded) // Either mobility has changed, or there are no components added.
					{
						StaticActorsWithNonStaticPrimitives.RemoveAtSwap(ActorIndex);
						--ActorIndex;
					}
				}

				// Build the dynamic data if required.
				for (AActor* Actor : Level->Actors)
				{
					// In the preprocessing step, we only handle static actors, to allow dynamic actors to update before insertion.
					if (Actor && !Actor->IsRootComponentStatic())
					{
						Actor->GetComponents<UPrimitiveComponent>(Primitives);
						for (UPrimitiveComponent* Primitive : Primitives)
						{
							check(Primitive);
							// Even if the component is static, it must be added in the dynamic list as the actor is not static.
							// This allows the static data to be reusable without processing when toggling visibility.
							if (Primitive->IsRegistered() && Primitive->SceneProxy)
							{
								DynamicComponentManager.Add(Primitive, LevelContext);
								DynamicComponents.Add(Primitive); // Track component added form this level so we can remove them later.
							}
						}
					}
				}
			}
			bIsInitialized = true;
		}
		else if (!Level->bIsVisible && bIsInitialized)
		{
			// Remove the dynamic data when the level is not visible anymore.
			for (const UPrimitiveComponent* Component : DynamicComponents)
			{
				DynamicComponentManager.Remove(Component, RemovedTextures);
			}	
			DynamicComponents.Empty();

			// Mark all static textures for removal.
			for (auto It = StaticInstances.GetTextureIterator(); It; ++It)
			{
				RemovedTextures.Push(*It);
			}

			bIsInitialized = false;
		}

		// If the level is visible, update the bounds.
		if (Level->bIsVisible)
		{
			StaticInstances.Refresh(Percentage);
		}
	}
}

uint32 FLevelTextureManager::GetAllocatedSize() const
{
	return StaticInstances.GetAllocatedSize() + 
		DynamicComponents.GetAllocatedSize() + 
		PendingInsertionStaticPrimitives.GetAllocatedSize() + 
		UnprocessedStaticComponents.GetAllocatedSize() + 
		PendingInsertionStaticPrimitives.GetAllocatedSize();
}
