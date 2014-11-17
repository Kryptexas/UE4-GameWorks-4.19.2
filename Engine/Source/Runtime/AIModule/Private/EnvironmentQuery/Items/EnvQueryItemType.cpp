// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/Items/EnvQueryItemType.h"

UEnvQueryItemType::UEnvQueryItemType(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	// register in known types 
	if (HasAnyFlags(RF_ClassDefaultObject) && !GetClass()->HasAnyClassFlags(CLASS_Abstract))
	{
		UEnvQueryManager::RegisteredItemTypes.Add(GetClass());
	}
}

void UEnvQueryItemType::FinishDestroy()
{
	// unregister from known types 
	UEnvQueryManager::RegisteredItemTypes.RemoveSingleSwap(GetClass());

	Super::FinishDestroy();
}

void UEnvQueryItemType::AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const
{
}

bool UEnvQueryItemType::StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const
{
	return false;
}

FString UEnvQueryItemType::GetDescription(const uint8* RawData) const
{
	return TEXT("item");
}
