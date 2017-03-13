// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/LazyObjectPtr.h"
#include "ControlRigEditModeSettings.generated.h"

class AActor;
class UControlRig;
class UControlRigSequence;

/** Settings object used to show useful information in the details panel */
UCLASS()
class UControlRigEditModeSettings : public UObject
{
	GENERATED_BODY()

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	/** Sequence to animate */
	UPROPERTY(EditAnywhere, Category = "Animation")
	UControlRigSequence* Sequence;

	/** The actor we are currently animating */
	UPROPERTY(EditAnywhere, Category = "Animation")
	TLazyObjectPtr<AActor> Actor;
};