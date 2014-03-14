// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavRelevantComponent.generated.h"

UCLASS(HeaderGroup=Navigation)
class ENGINE_API UNavRelevantComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	virtual void OnOwnerRegistered();
	virtual void OnOwnerUnregistered();

	virtual void OnApplyModifiers(struct FCompositeNavModifier& Modifiers);

	void SetNavigationRelevancy(bool bRelevant);
	bool IsNavigationRelevant() const { return bNavigationRelevant; }

	/** force refresh in navigation octree */
	void RefreshNavigationModifiers();

	/** bounds when owner is providing per component data */
	FBox Bounds;

protected:

	UPROPERTY()
	uint32 bNavigationRelevant : 1;
};
