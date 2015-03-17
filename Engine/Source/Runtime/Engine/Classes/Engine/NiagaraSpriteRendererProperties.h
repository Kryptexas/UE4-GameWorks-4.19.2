// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraEffectRendererProperties.h"
#include "NiagaraSpriteRendererProperties.generated.h"

UCLASS()
class UNiagaraSpriteRendererProperties : public UNiagaraEffectRendererProperties
{
	GENERATED_BODY()
public:
	UNiagaraSpriteRendererProperties(const FObjectInitializer& ObjectInitializer);
public:

	UNiagaraSpriteRendererProperties()
	{
		SubImageInfo = FVector2D(1.0f, 1.0f);
	}

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector2D SubImageInfo;
};
