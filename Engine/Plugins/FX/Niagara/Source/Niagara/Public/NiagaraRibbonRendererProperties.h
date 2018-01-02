// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraRibbonRendererProperties.generated.h"

UENUM()
enum class ENiagaraRibbonFacingMode : uint8
{
	Screen,
	Custom
};

UCLASS(editinlinenew)
class UNiagaraRibbonRendererProperties : public UNiagaraRendererProperties
{
public:
	GENERATED_BODY()

	UNiagaraRibbonRendererProperties()
		: Material(nullptr)
		, FacingMode(ENiagaraRibbonFacingMode::Screen)
		, UV0TilingDistance(0.0f)
		, UV0Scale(FVector2D(1.0f, 1.0f))
		, UV1TilingDistance(0.0f)
		, UV1Scale(FVector2D(1.0f, 1.0f))
	{
	}

	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
	UMaterialInterface* Material;

	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
	ENiagaraRibbonFacingMode FacingMode;

	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
	float UV0TilingDistance;
	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
	FVector2D UV0Scale;

	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
	float UV1TilingDistance;
	UPROPERTY(EditAnywhere, Category = "Ribbon Rendering")
	FVector2D UV1Scale;

	//~ UNiagaraRendererProperties interface
	virtual NiagaraRenderer* CreateEmitterRenderer(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const override;
#if WITH_EDITORONLY_DATA
	virtual const TArray<FNiagaraVariable>& GetRequiredAttributes() override;
	virtual const TArray<FNiagaraVariable>& GetOptionalAttributes() override;
    virtual bool IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage) override;
	virtual void FixMaterial(UMaterial* Material);
#endif
};
