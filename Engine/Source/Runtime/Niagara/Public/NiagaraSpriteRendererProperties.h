// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraEffectRendererProperties.h"
#include "NiagaraSpriteRendererProperties.generated.h"

UCLASS(editinlinenew)
class UNiagaraSpriteRendererProperties : public UNiagaraEffectRendererProperties
{
public:
	GENERATED_BODY()

	UNiagaraSpriteRendererProperties();

	//~ UNiagaraEffectRendererProperties interface
	virtual NiagaraEffectRenderer* CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel) override;
#if WITH_EDITORONLY_DATA
	virtual bool IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage) override;
	virtual void FixMaterial(UMaterial* Material) override;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector2D SubImageInfo;

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	bool bBVelocityAligned;
};
