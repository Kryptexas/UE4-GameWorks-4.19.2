// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkillSystemModulePrivatePCH.h"
#include "EngineKismetLibraryClasses.h"
#include "ParticleDefinitions.h"
#include "SoundDefinitions.h"

UGameplayCueView::UGameplayCueView(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}


void FGameplayCueHandler::GameplayCueActivated(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	check(Owner);
	for (FName Tag : GameplayCueTags.Tags)
	{
		for (UGameplayCueView * Def : Definitions)
		{
			for (FGameplayCueViewInfo & View : Def->Views)
			{
				if (View.CueType == EGameplayCueEvent::Applied && View.Tags.HasTag(Tag))
				{
					View.SpawnViewEffects(Owner, NULL);
				}
			}
		}
	}
}

void FGameplayCueHandler::GameplayCueExecuted(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	check(Owner);
	for (FName Tag : GameplayCueTags.Tags)
	{
		for (UGameplayCueView * Def : Definitions)
		{
			for (FGameplayCueViewInfo & View : Def->Views)
			{
				if (View.CueType == EGameplayCueEvent::Executed && View.Tags.HasTag(Tag))
				{
					View.SpawnViewEffects(Owner, NULL);
				}
			}
		}
	}
}

void FGameplayCueHandler::GameplayCueAdded(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	check(Owner);

	for (FName Tag : GameplayCueTags.Tags)
	{
		TArray<TSharedPtr<FGameplayCueViewEffects> > &Effects = SpawnedViewEffects.FindOrAdd(Tag);

		// Clear old effects if they existed? This will vary case to case. Removing old effects is the easiest approach
		ClearEffects(Effects);
		check(Effects.Num() == 0);

		// Add new effects
		for (UGameplayCueView * Def : Definitions)
		{
			for (FGameplayCueViewInfo & View : Def->Views)
			{
				if (View.CueType == EGameplayCueEvent::Added && View.Tags.HasTag(Tag))
				{
					TSharedPtr<FGameplayCueViewEffects> SpawnedEffects = View.SpawnViewEffects(Owner, &SpawnedObjects);
					Effects.Add(SpawnedEffects);
				}
			}
		}
	}
}

void FGameplayCueHandler::GameplayCueRemoved(const FGameplayTagContainer & GameplayCueTags, float NormalizedMagnitude)
{
	check(Owner);

	for (FName Tag : GameplayCueTags.Tags)
	{
		TArray<TSharedPtr<FGameplayCueViewEffects> > *Effects = SpawnedViewEffects.Find(Tag);
		if (Effects)
		{
			ClearEffects(*Effects);
			SpawnedViewEffects.Remove(Tag);
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
			Effect.AudioComponent->DestroyComponent();
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

TSharedPtr<FGameplayCueViewEffects> FGameplayCueViewInfo::SpawnViewEffects(AActor *Owner, TArray<UObject*> *SpawnedObjects) const
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
		SpawnedEffects->ParticleSystemComponent = UGameplayStatics::SpawnEmitterAttached(ParticleSystem, Owner->GetRootComponent());
		if (SpawnedObjects)
		{
			SpawnedObjects->Add(SpawnedEffects->ParticleSystemComponent.Get());
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