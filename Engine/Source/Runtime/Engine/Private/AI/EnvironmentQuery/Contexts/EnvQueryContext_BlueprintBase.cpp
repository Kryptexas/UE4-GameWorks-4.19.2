// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintNodeHelpers.h"

namespace
{
	FORCEINLINE bool DoesImplementBPFunction(FName FuncName, const UObject* Ob, const UClass* StopAtClass)
	{
		return (Ob->GetClass()->FindFunctionByName(FuncName)->GetOuter() != StopAtClass);
	}
}

UEnvQueryContext_BlueprintBase::UEnvQueryContext_BlueprintBase(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP), CallMode(UEnvQueryContext_BlueprintBase::InvalidCallMode)
{
	UClass* StopAtClass = UEnvQueryContext_BlueprintBase::StaticClass();	
	bool bImplementsProvideSingleActor = DoesImplementBPFunction(GET_FUNCTION_NAME_CHECKED(UEnvQueryContext_BlueprintBase, ProvideSingleActor), this, StopAtClass);
	bool bImplementsProvideSingleLocation = DoesImplementBPFunction(GET_FUNCTION_NAME_CHECKED(UEnvQueryContext_BlueprintBase, ProvideSingleLocation), this, StopAtClass);
	bool bImplementsProvideActorSet = DoesImplementBPFunction(GET_FUNCTION_NAME_CHECKED(UEnvQueryContext_BlueprintBase, ProvideActorsSet), this, StopAtClass);
	bool bImplementsProvideLocationsSet = DoesImplementBPFunction(GET_FUNCTION_NAME_CHECKED(UEnvQueryContext_BlueprintBase, ProvideLocationsSet), this, StopAtClass);

	int32 ImplementationsCount = 0;
	if (bImplementsProvideSingleActor)
	{
		++ImplementationsCount;
		CallMode = SingleActor;
	}

	if (bImplementsProvideSingleLocation)
	{
		++ImplementationsCount;
		CallMode = SingleLocation;
	}

	if (bImplementsProvideActorSet)
	{
		++ImplementationsCount;
		CallMode = ActorSet;
	}

	if (bImplementsProvideLocationsSet)
	{
		++ImplementationsCount;
		CallMode = LocationSet;
	}

	if (ImplementationsCount != 1)
	{

	}
}

void UEnvQueryContext_BlueprintBase::ProvideContext(struct FEnvQueryInstance& QueryInstance, struct FEnvQueryContextData& ContextData) const
{
	AActor* QuerierActor = Cast<AActor>(QueryInstance.Owner.Get());

	if (QuerierActor == NULL || CallMode == InvalidCallMode)
	{
		return;
	}
	
	switch (CallMode)
	{
	case SingleActor:
		{
			AActor* ResultingActor = NULL;
			ProvideSingleActor(QuerierActor, ResultingActor);
			UEnvQueryItemType_Actor::SetContextHelper(ContextData, ResultingActor);
		}
		break;
	case SingleLocation:
		{
			FVector ResultingLocation = FAISystem::InvalidLocation;
			ProvideSingleLocation(QuerierActor, ResultingLocation);
			UEnvQueryItemType_Point::SetContextHelper(ContextData, ResultingLocation);
		}
		break;
	case ActorSet:
		{
			TArray<AActor*> ActorSet;
			ProvideActorsSet(QuerierActor, ActorSet);
			UEnvQueryItemType_Actor::SetContextHelper(ContextData, ActorSet);
		}
		break;
	case LocationSet:
		{
			TArray<FVector> LocationSet;
			ProvideLocationsSet(QuerierActor, LocationSet);
			UEnvQueryItemType_Point::SetContextHelper(ContextData, LocationSet);
		}
		break;
	default:
		break;
	}
}
