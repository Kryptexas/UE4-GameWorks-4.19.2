// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ComponentInstanceDataCache.h"

FComponentInstanceDataBase::FComponentInstanceDataBase(const UActorComponent* SourceComponent)
{
	check(SourceComponent);
	SourceComponentName = SourceComponent->GetFName();
	SourceComponentClass = SourceComponent->GetClass();
	SourceComponentTypeSerializedIndex = -1;
	
	AActor* ComponentOwner = SourceComponent->GetOwner();
	if (ComponentOwner)
	{
		bool bFound = false;
		for (const UActorComponent* BlueprintCreatedComponent : ComponentOwner->BlueprintCreatedComponents)
		{
			if (BlueprintCreatedComponent)
			{
				if (BlueprintCreatedComponent == SourceComponent)
				{
					++SourceComponentTypeSerializedIndex;
					bFound = true;
					break;
				}
				else if (BlueprintCreatedComponent->GetClass() == SourceComponentClass)
				{
					++SourceComponentTypeSerializedIndex;
				}
			}
		}
		if (!bFound)
		{
			SourceComponentTypeSerializedIndex = -1;
		}
	}
}

bool FComponentInstanceDataBase::MatchesComponent(const UActorComponent* Component) const
{
	bool bMatches = false;
	if (Component && Component->GetClass() == SourceComponentClass)
	{
		if (Component->GetFName() == SourceComponentName)
		{
			bMatches = true;
		}
		else if (SourceComponentTypeSerializedIndex >= 0)
		{
			int32 FoundSerializedComponentsOfType = -1;
			AActor* ComponentOwner = Component->GetOwner();
			if (ComponentOwner)
			{
				for (const UActorComponent* BlueprintCreatedComponent : ComponentOwner->BlueprintCreatedComponents)
				{
					if (   BlueprintCreatedComponent
						&& (BlueprintCreatedComponent->GetClass() == SourceComponentClass)
						&& (++FoundSerializedComponentsOfType == SourceComponentTypeSerializedIndex))
					{
						bMatches = (BlueprintCreatedComponent == Component);
						break;
					}
				}
			}
		}
	}
	return bMatches;
}

FComponentInstanceDataCache::FComponentInstanceDataCache(const AActor* Actor)
{
	if(Actor != NULL)
	{
		TInlineComponentArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		// Grab per-instance data we want to persist
		for (UActorComponent* Component : Components)
		{
			if (Component->CreationMethod == EComponentCreationMethod::ConstructionScript) // Only cache data from 'created by construction script' components
			{
				FComponentInstanceDataBase* ComponentInstanceData = Component->GetComponentInstanceData();
				if (ComponentInstanceData)
				{
					check(!Component->GetComponentInstanceDataType().IsNone());
					TypeToDataMap.Add(Component->GetComponentInstanceDataType(), ComponentInstanceData);
				}
			}
			else if (Component->CreationMethod == EComponentCreationMethod::Instance)
			{
				// If the instance component is attached to a BP component we have to be prepared for the possibility that it will be deleted
				if (USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
				{
					if (SceneComponent->AttachParent && SceneComponent->AttachParent->CreationMethod == EComponentCreationMethod::ConstructionScript)
					{
						InstanceComponentTransformToRootMap.Add(SceneComponent, SceneComponent->GetComponentTransform().GetRelativeTransform(Actor->GetRootComponent()->GetComponentTransform()));
					}
				}
			}
		}
	}
}

FComponentInstanceDataCache::~FComponentInstanceDataCache()
{
	for (auto InstanceDataPair : TypeToDataMap)
	{
		delete InstanceDataPair.Value;
	}
}

void FComponentInstanceDataCache::ApplyToActor(AActor* Actor) const
{
	if(Actor != NULL)
	{
		TInlineComponentArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		// Apply per-instance data.
		for (UActorComponent* Component : Components)
		{
			if(Component->CreationMethod == EComponentCreationMethod::ConstructionScript) // Only try and apply data to 'created by construction script' components
			{
				const FName ComponentInstanceDataType = Component->GetComponentInstanceDataType();

				if (!ComponentInstanceDataType.IsNone())
				{
					TArray< FComponentInstanceDataBase* > CachedData;
					TypeToDataMap.MultiFind(ComponentInstanceDataType, CachedData);

					for (FComponentInstanceDataBase* ComponentInstanceData : CachedData)
					{
						if (ComponentInstanceData && ComponentInstanceData->MatchesComponent(Component))
						{
							ComponentInstanceData->ApplyToComponent(Component);
							break;
						}
					}
				}
			}
		}

		// Once we're done attaching, if we have any unattached instance components move them to the root
		for (auto InstanceTransformPair : InstanceComponentTransformToRootMap)
		{
			check(Actor->GetRootComponent());

			USceneComponent* SceneComponent = InstanceTransformPair.Key;
			if (SceneComponent && (SceneComponent->AttachParent == nullptr || SceneComponent->AttachParent->IsPendingKill()))
			{
				SceneComponent->AttachTo(Actor->GetRootComponent());
				SceneComponent->SetRelativeTransform(InstanceTransformPair.Value);
			}
		}
	}
}

void FComponentInstanceDataCache::FindAndReplaceInstances(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	for (auto ComponentInstanceDataPair : TypeToDataMap)
	{
		if (ComponentInstanceDataPair.Value)
		{
			ComponentInstanceDataPair.Value->FindAndReplaceInstances(OldToNewInstanceMap);
		}
	}
	TArray<USceneComponent*> SceneComponents;
	InstanceComponentTransformToRootMap.GetKeys(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (UObject* const* NewSceneComponent = OldToNewInstanceMap.Find(SceneComponent))
		{
			if (*NewSceneComponent)
			{
				InstanceComponentTransformToRootMap.Add(CastChecked<USceneComponent>(*NewSceneComponent), InstanceComponentTransformToRootMap.FindAndRemoveChecked(SceneComponent));
			}
			else
			{
				InstanceComponentTransformToRootMap.Remove(SceneComponent);
			}
		}
	}
}