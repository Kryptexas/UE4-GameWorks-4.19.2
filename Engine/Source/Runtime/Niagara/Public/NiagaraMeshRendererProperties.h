// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraEffectRendererProperties.h"
#include "StaticMeshResources.h"
#include "NiagaraMeshRendererProperties.generated.h"

UCLASS(editinlinenew)
class UNiagaraMeshRendererProperties : public UNiagaraEffectRendererProperties
{
public:
	GENERATED_BODY()

	UNiagaraMeshRendererProperties();

	//~ UNiagaraEffectRendererProperties interface
	virtual NiagaraEffectRenderer* CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel) override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const override;

#if WITH_EDITORONLY_DATA
	virtual bool IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage) override;
	virtual void FixMaterial(UMaterial* Material) override;
#endif // WITH_EDITORONLY_DATA

	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	UPROPERTY(EditAnywhere, Category = "Mesh Rendering")
	UStaticMesh *ParticleMesh;
};
