// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraEffectRendererProperties.h"
#include "NiagaraSpriteRendererProperties.generated.h"


UENUM()
enum class ENiagaraSortMode : uint8
{
	SortNone,
	SortViewDepth,
	SortViewDistance
};

UENUM()
enum class ENiagaraSpriteAlignment : uint8
{
	Unaligned,
	VelocityAligned,
	CustomAlignment
};


UENUM()
enum class ENiagaraSpriteFacingMode : uint8
{
	FaceCamera,
	FaceCameraPlane,
	CustomFacingVector
};


UCLASS(editinlinenew)
class UNiagaraSpriteRendererProperties : public UNiagaraEffectRendererProperties
{
public:
	GENERATED_BODY()

	UNiagaraSpriteRendererProperties();

	//~ UNiagaraEffectRendererProperties interface
	virtual NiagaraEffectRenderer* CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const override;
#if WITH_EDITORONLY_DATA
	virtual bool IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage) override;
	virtual void FixMaterial(UMaterial* Material) override;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector2D SubImageSize;

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	ENiagaraSpriteAlignment Alignment;

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	ENiagaraSpriteFacingMode FacingMode;

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	FVector CustomFacingVectorMask;

	UPROPERTY(EditAnywhere, Category = "Sprite Rendering")
	ENiagaraSortMode SortMode;
};



