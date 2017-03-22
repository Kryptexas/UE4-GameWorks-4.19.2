//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include "PhononSettings.h"
#include "Components/ActorComponent.h"
#include "PhononMaterialComponent.generated.h"

/**
 * Phonon Material components are used to customize an actor's acoustic properties. Only valid on actors that also
 * have a Phonon Geometry component.
 */
UCLASS(ClassGroup = (Audio), HideCategories = (Activation, Collision), meta = (BlueprintSpawnableComponent))
class UPhononMaterialComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPhononMaterialComponent();

	IPLMaterial GetMaterialPreset() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif

	UPROPERTY()
	int32 MaterialIndex;

	UPROPERTY(EditAnywhere, Category = Settings)
	EPhononMaterial MaterialPreset;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float LowFreqAbsorption;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MidFreqAbsorption;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HighFreqAbsorption;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Scattering;
};