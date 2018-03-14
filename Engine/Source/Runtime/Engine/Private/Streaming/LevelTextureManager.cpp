// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LevelTextureManager.cpp: Implementation of content streaming classes.
=============================================================================*/

#include "Streaming/LevelTextureManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

void FLevelTextureManager::Remove(FRemovedTextureArray* RemovedTextures)
{ 
	TArray<const UPrimitiveComponent*> ReferencedComponents;
	StaticInstances.GetReferencedComponents(ReferencedComponents);
	ReferencedComponents.Append(UnprocessedComponents);
	ReferencedComponents.Append(PendingInsertionStaticPrimitives);
	for (const UPrimitiveComponent* Component : ReferencedComponents)
	{
		if (Component)
		{
			check(Component->IsValidLowLevelFast()); // Check that this component was not already destroyed.
			check(Component->bAttachedToStreamingManagerAsStatic);  // Check that is correctly tracked

			// A component can only be referenced in one level, so if it was here, we can clear the flag
			Component->bAttachedToStreamingManagerAsStatic = false;
		}
	}

	// Mark all static textures for removal.
	if (RemovedTextures)
	{
		for (auto It = StaticInstances.GetTextureIterator(); It; ++It)
		{
			RemovedTextures->Push(*It);
		}
	}

	BuildStep = EStaticBuildStep::BuildTextureLookUpMap;
	UnprocessedStaticActors.Empty();
	UnprocessedComponents.Empty();
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

bool FLevelTextureManager::AddPrimitive(FDynamicTextureInstanceManager& DynamicComponentManager, FStreamingTextureLevelContext& LevelContext, const UPrimitiveComponent* Primitive, bool bLevelIsVisible, float MaxTextureUVDensity)
{
	check(!Primitive->bAttachedToStreamingManagerAsStatic);

	if (Primitive->Mobility == EComponentMobility::Static)
	{
		// Once the level becomes visible, static component not registered are considered invalid.
		if (bLevelIsVisible && !Primitive->IsRegistered())
		{
			return false;
		}

		switch (StaticInstances.Add(Primitive, LevelContext, MaxTextureUVDensity))
		{
		case EAddComponentResult::Success:
			Primitive->bAttachedToStreamingManagerAsStatic = true;
			// Remove it from the dynamic list, if it was there.
			DynamicComponentManager.Remove(Primitive, nullptr);
			Primitive->bHandledByStreamingManagerAsDynamic = false;
			return true;
		case EAddComponentResult::Fail_UIDensityConstraint:
			if (!Primitive->bHandledByStreamingManagerAsDynamic)
			{
				// Static components with big UIDensites are handled as dynamic, otherwise they break the r.Streaming.MinLevelTextureScreenSize optimization.
				DynamicComponentManager.Add(Primitive, LevelContext);
			}
			return true;
		default:
			// Don't know what to do, as there could be several case for this.
			return false;
		}
	}
	else // Not a static primitive
	{
		if (!Primitive->bHandledByStreamingManagerAsDynamic)
		{
			DynamicComponentManager.Add(Primitive, LevelContext);
		}
		return true;
	}
}

void FLevelTextureManager::IncrementalBuild(FDynamicTextureInstanceManager& DynamicComponentManager, FStreamingTextureLevelContext& LevelContext, bool bForceCompletion, int64& NumStepsLeft)
{
	check(Level);

	const float MaxTextureUVDensity = CVarStreamingMaxTextureUVDensity.GetValueOnAnyThread();

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
		// Those must be cleared at this point.
		check(UnprocessedStaticActors.Num() == 0 && UnprocessedComponents.Num() == 0 && PendingInsertionStaticPrimitives.Num() == 0);

		// Find all static actors, all actors created after this point, static or dynamic, must go through the spawn actor logic, which will make then dynamic.
		// If the mobility goes from static to dynamic after this, components will be removed from the static instances in the last stage.
		// If the mobility changes from dynamic to static, then this is not handled. @todo
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

			// Check if the actor is still static, as the mobility could change.
			if (StaticActor->IsRootComponentStatic())
			{
				TInlineComponentArray<UPrimitiveComponent*> Primitives;
				StaticActor->GetComponents<UPrimitiveComponent>(Primitives);

				// Append all primitives, static and dynamic will be sorted out in the next stage.
				UnprocessedComponents.Append(Primitives);
				for (const UPrimitiveComponent* Primitive : Primitives)
				{
					check(Primitive);
					Primitive->bAttachedToStreamingManagerAsStatic = true;
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
		while ((bForceCompletion || NumStepsLeft > 0) && UnprocessedComponents.Num())
		{
			const UPrimitiveComponent* Primitive = UnprocessedComponents.Pop(false);
			check(Primitive);
			Primitive->bAttachedToStreamingManagerAsStatic = false;

			if (!AddPrimitive(DynamicComponentManager, LevelContext, Primitive, Level->bIsVisible, MaxTextureUVDensity))
			{
				// If it failed, reprocess it later if it was not in a fully valid state.
				if (!Primitive->IsRegistered() || !Primitive->IsRenderStateCreated())
				{
					PendingInsertionStaticPrimitives.Add(Primitive);
					Primitive->bAttachedToStreamingManagerAsStatic = true;
				}
			}
			--NumStepsLeft;
		}

		if (!UnprocessedComponents.Num())
		{
			UnprocessedComponents.Empty(); // Free the memory.
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
			TArray<const UPrimitiveComponent*> RemovedComponents;

			// Remove unregistered component and resolve the bounds using the packed relative boxes.
			NumStepsLeft -= (int64)StaticInstances.CheckRegistrationAndUnpackBounds(RemovedComponents);

			for (const UPrimitiveComponent* Component : RemovedComponents)
			{	// Those components were released, not referenced anymore.
				check(Component);				
				Component->bAttachedToStreamingManagerAsStatic = false;
			}

			NumStepsLeft -= (int64)PendingInsertionStaticPrimitives.Num();

			// Reprocess the components that didn't have valid data.
			while (PendingInsertionStaticPrimitives.Num())
			{
				const UPrimitiveComponent* Primitive = PendingInsertionStaticPrimitives.Pop(false);
				check(Primitive);
				Primitive->bAttachedToStreamingManagerAsStatic = false; // Clear now since not referenced anymore.
				
				AddPrimitive(DynamicComponentManager, LevelContext, Primitive, true, MaxTextureUVDensity);
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
			IncrementalBuild(DynamicComponentManager, LevelContext, Level->bIsVisible, NumStepsLeftForIncrementalBuild);
		}
		while (NeedsIncrementalBuild(NumStepsLeftForIncrementalBuild));
	}

	if (BuildStep == EStaticBuildStep::Done)
	{
		if (Level->bIsVisible && !bIsInitialized)
		{
			FStreamingTextureLevelContext LevelContext(EMaterialQualityLevel::Num, Level);
			// Flag all dynamic components so that they get processed.
			for (const AActor* Actor : Level->Actors)
			{
				// In the preprocessing step, we only handle static actors, to allow dynamic actors to update before insertion.
				if (Actor && !Actor->IsRootComponentStatic())
				{
					TInlineComponentArray<UPrimitiveComponent*> Primitives;
					Actor->GetComponents<UPrimitiveComponent>(Primitives);
					for (UPrimitiveComponent* Primitive : Primitives)
					{
						check(Primitive);

						// Because the root component mobility could have changed, it is possible for a components to be attached as static here.
						if (Primitive->bAttachedToStreamingManagerAsStatic)
						{
							StaticInstances.Remove(Primitive, nullptr); 
							Primitive->bAttachedToStreamingManagerAsStatic = false;
						}

						// If the flag is already set, then this primitive is already handled when it's proxy get created.
						if (!Primitive->bHandledByStreamingManagerAsDynamic)
						{
							DynamicComponentManager.Add(Primitive, LevelContext);
						}
					}
				}
			}
			bIsInitialized = true;
		}
		else if (!Level->bIsVisible && bIsInitialized)
		{
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

void FLevelTextureManager::NotifyLevelOffset(const FVector& Offset)
{
	if (BuildStep == EStaticBuildStep::Done)
	{
		// offset static primitives bounds
		StaticInstances.OffsetBounds(Offset);
	}
}

uint32 FLevelTextureManager::GetAllocatedSize() const
{
	return StaticInstances.GetAllocatedSize() + 
		UnprocessedStaticActors.GetAllocatedSize() + 
		UnprocessedComponents.GetAllocatedSize() + 
		PendingInsertionStaticPrimitives.GetAllocatedSize();
}
