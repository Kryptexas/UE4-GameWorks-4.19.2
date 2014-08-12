// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayAbilityWorldReticle.generated.h"

class AGameplayAbilityTargetActor;

UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API AGameplayAbilityWorldReticle : public AActor
{
	GENERATED_UCLASS_BODY()

public:

	/** Accessor for checking, before instantiating, if this WorldReticle will replicate. */
	bool GetReplicates() const;

	// ------------------------------

	virtual bool IsNetRelevantFor(class APlayerController* RealViewer, AActor* Viewer, const FVector& SrcLocation) override;

	void InitializeReticle(AGameplayAbilityTargetActor* InTargetingActor);

protected:
	/** This is used in the process of determining whether we should replicate to a specific client. */
	UPROPERTY(BlueprintReadOnly, Category = "Network")
	APlayerController* MasterPC;

	/** In the future, we may want to grab things like sockets off of this. */
	UPROPERTY(BlueprintReadOnly, Category = "Network")
	AGameplayAbilityTargetActor* TargetingActor;
};
