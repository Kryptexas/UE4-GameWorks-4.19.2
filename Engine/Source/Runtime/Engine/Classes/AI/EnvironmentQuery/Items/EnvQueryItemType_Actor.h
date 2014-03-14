// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryItemType_Actor.generated.h"

UCLASS()
class ENGINE_API UEnvQueryItemType_Actor : public UEnvQueryItemType_ActorBase
{
	GENERATED_UCLASS_BODY()

	static AActor* GetValue(const uint8* RawData);
	static void SetValue(uint8* RawData, const AActor* Value);

	static void SetContextHelper(struct FEnvQueryContextData& ContextData, const AActor* SingleActor);
	static void SetContextHelper(struct FEnvQueryContextData& ContextData, const TArray<const AActor*>& MultipleActors);

protected:

	FVector GetActorLocation(const uint8* RawData);
	FRotator GetActorRotation(const uint8* RawData);
	AActor* GetActor(const uint8* RawData);
};
