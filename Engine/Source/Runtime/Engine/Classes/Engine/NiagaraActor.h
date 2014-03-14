// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraActor.generated.h"

UCLASS()
class ANiagaraActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Pointer to effect component */
	UPROPERTY(VisibleAnywhere, Category=NiagaraActor)
	TSubobjectPtr<class UNiagaraComponent> NiagaraComponent;

#if WITH_EDITORONLY_DATA
	// Reference to sprite visualization component
	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> SpriteComponent;

	// Reference to arrow visualization component
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;
#endif
};
