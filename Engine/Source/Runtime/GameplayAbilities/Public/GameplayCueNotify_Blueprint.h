// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayCueNotify_Blueprint.generated.h"


/**
 *	Instanced GameplayCueNotify which runs arbitrary blueprint code.
 */

UCLASS(Blueprintable,meta=(ShowWorldContextPin))
class GAMEPLAYABILITIES_API UGameplayCueNotify_Blueprint : public UGameplayCueNotify
{
	GENERATED_UCLASS_BODY()

	/** Does this GameplayCueNotify handle this type of GameplayCueEvent? */
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;

	/** Does this GameplayCueNotify need a per source actor instance for this event? */
	virtual bool NeedsInstanceForEvent(EGameplayCueEvent::Type EventType) const override;

	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters) override;

	/** Event Graph  */
	UFUNCTION(BlueprintImplementableEvent, Category="GameplayCueNotify", FriendlyName="HandleGameplayCue")
	void K2_HandleGameplayCue(TWeakObjectPtr<AActor> MyTarget, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);

	UFUNCTION(BlueprintImplementableEvent)
	bool OnExecute(TWeakObjectPtr<AActor> MyTarget, FGameplayCueParameters Parameters);

	UFUNCTION(BlueprintImplementableEvent)
	bool OnActive(TWeakObjectPtr<AActor> MyTarget, FGameplayCueParameters Parameters);

	UFUNCTION(BlueprintImplementableEvent)
	bool OnRemove(TWeakObjectPtr<AActor> MyTarget, FGameplayCueParameters Parameters);
};