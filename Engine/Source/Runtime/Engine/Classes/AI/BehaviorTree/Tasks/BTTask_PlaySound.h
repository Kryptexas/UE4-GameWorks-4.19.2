// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_PlaySound.generated.h"

UCLASS()
class UBTTask_PlaySound : public UBTTaskNode
{
	GENERATED_UCLASS_BODY()

	/** CUE to play */
	UPROPERTY(Category=Node, EditAnywhere)
	class USoundCue* SoundToPlay;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;
};
