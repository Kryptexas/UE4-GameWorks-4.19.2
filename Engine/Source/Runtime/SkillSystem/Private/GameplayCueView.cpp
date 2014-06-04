// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkillSystemModulePrivatePCH.h"
#include "GameplayEffect.h"
#include "GameplayCueView.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameplayTagsModule.h"

UGameplayCueView::UGameplayCueView(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}

FGameplayCueViewInfo * FGameplayCueHandler::GetBestMatchingView(EGameplayCueEvent::Type Type, const FGameplayTag BaseTag)
{
	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	FGameplayTagContainer TagAndParentsContainer = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTagParents(BaseTag);

	for (auto InnerTagIt = TagAndParentsContainer.CreateConstIterator(); InnerTagIt; ++InnerTagIt)
	{
		FGameplayTag Tag = *InnerTagIt;

		for (UGameplayCueView * Def : Definitions)
		{
			for (FGameplayCueViewInfo & View : Def->Views)
			{
				if (View.CueType == Type && View.Tags.HasTag(Tag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit))
				{
					return &View;
				}
			}
		}
	}

	return NULL;
}

void FGameplayCueHandler::GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	check(Owner);
	for (auto TagIt = GameplayCueTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		if (FGameplayCueViewInfo * View = GetBestMatchingView(EGameplayCueEvent::Applied, *TagIt))
		{
			View->SpawnViewEffects(Owner, NULL, InstigatorContext);
		}
	}
}

void FGameplayCueHandler::GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	check(Owner);
	for (auto TagIt = GameplayCueTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		if (FGameplayCueViewInfo * View = GetBestMatchingView(EGameplayCueEvent::Executed, *TagIt))
		{
			View->SpawnViewEffects(Owner, NULL, InstigatorContext);
		}
	}
}

void FGameplayCueHandler::GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	check(Owner);

	for (auto TagIt = GameplayCueTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		TArray<TSharedPtr<FGameplayCueViewEffects> > &Effects = SpawnedViewEffects.FindOrAdd(*TagIt);

		// Clear old effects if they existed? This will vary case to case. Removing old effects is the easiest approach
		ClearEffects(Effects);
		check(Effects.Num() == 0);

		if (FGameplayCueViewInfo * View = GetBestMatchingView(EGameplayCueEvent::Added, *TagIt))
		{
			View->SpawnViewEffects(Owner, NULL, InstigatorContext);

			TSharedPtr<FGameplayCueViewEffects> SpawnedEffects = View->SpawnViewEffects(Owner, &SpawnedObjects, InstigatorContext);
			Effects.Add(SpawnedEffects);
		}
	}
}

void FGameplayCueHandler::GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude, const FGameplayEffectInstigatorContext InstigatorContext)
{
	check(Owner);

	for (auto TagIt = GameplayCueTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		TArray<TSharedPtr<FGameplayCueViewEffects> > *Effects = SpawnedViewEffects.Find(*TagIt);
		if (Effects)
		{
			ClearEffects(*Effects);
			SpawnedViewEffects.Remove(*TagIt);
		}	
		
		// Remove old effects
		
		if (FGameplayCueViewInfo * View = GetBestMatchingView(EGameplayCueEvent::Removed, *TagIt))
		{
			View->SpawnViewEffects(Owner, NULL, InstigatorContext);
		}
	}
}

void FGameplayCueHandler::ClearEffects(TArray< TSharedPtr<FGameplayCueViewEffects> > &Effects)
{
	bool RemovedSomething = false;
	for (TSharedPtr<FGameplayCueViewEffects> &EffectPtr : Effects)
	{
		if (!EffectPtr.IsValid())
		{
			continue;
		}

		FGameplayCueViewEffects &Effect = *EffectPtr.Get();

		if (Effect.SpawnedActor.IsValid())
		{
			Effect.SpawnedActor->Destroy();
			RemovedSomething = true;
		}

		if (Effect.AudioComponent.IsValid())
		{
			Effect.AudioComponent->Stop();
			RemovedSomething = true;
		}

		if (Effect.ParticleSystemComponent.IsValid())
		{
			Effect.ParticleSystemComponent->DestroyComponent();
			RemovedSomething = true;
		}
	}

	Effects.Empty();
	
	// Cleanup flat array
	if (RemovedSomething)
	{
		for (int32 idx=0; idx < SpawnedObjects.Num(); ++idx)
		{
			UObject *Obj = SpawnedObjects[idx];
			if (!Obj || Obj->IsPendingKill())
			{
				SpawnedObjects.RemoveAtSwap(idx);
				idx--;
			}
		}
	}
}

TSharedPtr<FGameplayCueViewEffects> FGameplayCueViewInfo::SpawnViewEffects(AActor *Owner, TArray<UObject*> *SpawnedObjects, const FGameplayEffectInstigatorContext InstigatorContext) const
{
	check(Owner);

	TSharedPtr<FGameplayCueViewEffects> SpawnedEffects = TSharedPtr<FGameplayCueViewEffects>(new FGameplayCueViewEffects());

	if (Sound)
	{
		SpawnedEffects->AudioComponent = UGameplayStatics::PlaySoundAttached(Sound, Owner->GetRootComponent());
		if (SpawnedObjects)
		{
			SpawnedObjects->Add(SpawnedEffects->AudioComponent.Get());
		}
	}
	if (ParticleSystem)
	{
		if (InstigatorContext.HitResult.IsValid())
		{
			FHitResult &HitResult = *InstigatorContext.HitResult.Get();

			SpawnedEffects->ParticleSystemComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSystem, Owner->GetRootComponent(), NAME_None, HitResult.Location,
				HitResult.Normal.Rotation(), EAttachLocation::KeepWorldPosition);
		}
		else
		{
			SpawnedEffects->ParticleSystemComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSystem, Owner->GetRootComponent());
		}		

		if (SpawnedObjects)
		{
			SpawnedObjects->Add(SpawnedEffects->ParticleSystemComponent.Get());
		}
		else if (SpawnedEffects->ParticleSystemComponent.IsValid())
		{
			for (int32 EmitterIndx = 0; EmitterIndx < SpawnedEffects->ParticleSystemComponent->EmitterInstances.Num(); EmitterIndx++)
			{
				if (SpawnedEffects->ParticleSystemComponent->EmitterInstances[EmitterIndx] &&
					SpawnedEffects->ParticleSystemComponent->EmitterInstances[EmitterIndx]->CurrentLODLevel &&
					SpawnedEffects->ParticleSystemComponent->EmitterInstances[EmitterIndx]->CurrentLODLevel->RequiredModule &&
					SpawnedEffects->ParticleSystemComponent->EmitterInstances[EmitterIndx]->CurrentLODLevel->RequiredModule->EmitterLoops == 0)
				{
					SKILL_LOG(Warning, TEXT("%s - particle system has a looping emitter. This should not be used in a executed GameplayCue!"), *SpawnedEffects->ParticleSystemComponent->GetName());
					break;
				}
			}
		}
	}
	if (ActorClass)
	{
		FVector Location = Owner->GetActorLocation();
		FRotator Rotation = Owner->GetActorRotation();
		SpawnedEffects->SpawnedActor = Owner->GetWorld()->SpawnActor(ActorClass, &Location, &Rotation);
		if (SpawnedObjects)
		{
			SpawnedObjects->Add(SpawnedEffects->SpawnedActor.Get());
		}
	}

	return SpawnedEffects;
}
