// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DynamicBlueprintBinding.generated.h"


UCLASS(abstract)
class ENGINE_API UDynamicBlueprintBinding : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual void BindDynamicDelegates(AActor* InInstance) const { }
};
