// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/ActorComponent.h"
#include "NavRelevantInterface.h"
#include "NavRelevantComponent.generated.h"

UCLASS()
class ENGINE_API UNavRelevantComponent : public UActorComponent, public INavRelevantInterface
{
	GENERATED_UCLASS_BODY()

	// Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	// End UActorComponent Interface

	// Begin INavRelevantInterface Interface
	virtual FBox GetNavigationBounds() const override;
	virtual bool IsNavigationRelevant() const override;
	virtual void UpdateNavigationBounds() override;
	virtual UObject* GetNavigationParent() const override;
	// End INavRelevantInterface Interface

	DEPRECATED(4.8, "This function is deprecated. Use CalcAndCacheBounds instead")
	virtual void CalcBounds();
	
	virtual void CalcAndCacheBounds() const;

	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	void SetNavigationRelevancy(bool bRelevant);

	/** force refresh in navigation octree */
	void RefreshNavigationModifiers();
	
protected:

	/** bounds for navigation octree */
	mutable FBox Bounds;

	UPROPERTY()
	uint32 bNavigationRelevant : 1;
};
