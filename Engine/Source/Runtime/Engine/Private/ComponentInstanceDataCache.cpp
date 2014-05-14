// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

FComponentInstanceDataBase::FComponentInstanceDataBase(const UActorComponent* SourceComponent)
{
	check(SourceComponent);
	SourceComponentName = SourceComponent->GetFName();
	SourceComponentTypeSerializedIndex = -1;
	
	AActor* ComponentOwner = SourceComponent->GetOwner();
	if (ComponentOwner)
	{
		bool bFound = false;
		for (const UActorComponent* SerializedComponent : ComponentOwner->SerializedComponents)
		{
			if (SerializedComponent == SourceComponent)
			{
				++SourceComponentTypeSerializedIndex;
				bFound = true;
				break;
			}
			else if (SerializedComponent->GetClass() == SourceComponent->GetClass())
			{
				++SourceComponentTypeSerializedIndex;
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
	if (Component)
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
				for (const UActorComponent* SerializedComponent : ComponentOwner->SerializedComponents)
				{
					if (   (SerializedComponent->GetClass() == Component->GetClass())
						&& (++FoundSerializedComponentsOfType == SourceComponentTypeSerializedIndex))
					{
						bMatches = true;
						break;
					}
				}
			}
		}
	}
	return bMatches;
}

void FComponentInstanceDataCache::AddInstanceData(TSharedPtr<FComponentInstanceDataBase> NewData)
{
	check(NewData.IsValid());
	FName TypeName = NewData->GetDataTypeName();
	TypeToDataMap.Add(TypeName, NewData);
}

void FComponentInstanceDataCache::GetInstanceDataOfType(FName TypeName, TArray< TSharedPtr<FComponentInstanceDataBase> >& OutData) const
{
	TypeToDataMap.MultiFind(TypeName, OutData);
}


void FComponentInstanceDataCache::GetFromActor(AActor* Actor, FComponentInstanceDataCache& Cache)
{
	if(Actor != NULL)
	{
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		// Grab per-instance data we want to persist
		for (int32 CompIdx=0; CompIdx<Components.Num(); CompIdx++)
		{
			UActorComponent* Component = Components[CompIdx];
			if(Component->bCreatedByConstructionScript) // Only cache data from 'created by construction script' components
			{
				Component->GetComponentInstanceData(Cache);
			}
		}
	}
}

void FComponentInstanceDataCache::ApplyToActor(AActor* Actor)
{
	if(Actor != NULL)
	{
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		// Apply per-instance data.
		for (int32 CompIdx=0; CompIdx<Components.Num(); CompIdx++)
		{
			UActorComponent* Component = Components[CompIdx];
			if(Component->bCreatedByConstructionScript) // Only try and apply data to 'created by construction script' components
			{
				Component->ApplyComponentInstanceData(*this);
			}
		}
	}
}