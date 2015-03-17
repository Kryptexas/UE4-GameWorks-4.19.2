// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_PlaySound.generated.h"

class UBehaviorTreeComponent;
class USoundCue;

/**
 * Play Sound task node.
 * Plays the specified sound when executed.
 */
UCLASS()
class AIMODULE_API UBTTask_PlaySound : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_PlaySound(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** CUE to play */
	UPROPERTY(Category=Node, EditAnywhere)
	USoundCue* SoundToPlay;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
};
