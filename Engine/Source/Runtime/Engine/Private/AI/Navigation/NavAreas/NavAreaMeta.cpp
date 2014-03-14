// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UNavAreaMeta::UNavAreaMeta(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

TSubclassOf<UNavArea> UNavAreaMeta::PickAreaClass(const class AActor* Actor, const FNavAgentProperties& NavAgent) const 
{ 
	UE_LOG(LogNavigation, Warning, TEXT("UNavAreaMeta::PickAreaClass called. UNavAreaMeta is an abstract class and should never get used directly!"));

	return GetClass(); 
}

int8 UNavAreaMeta::GetNavAgentIndex(const FNavAgentProperties& NavAgent) const
{
	const UNavigationSystem* DefNavSys = (UNavigationSystem*)(UNavigationSystem::StaticClass()->GetDefaultObject());
	return DefNavSys->GetSupportedAgentIndex(NavAgent);
}
