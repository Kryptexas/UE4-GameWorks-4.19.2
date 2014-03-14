// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PointLight.generated.h"

UCLASS(HeaderGroup=Light, ClassGroup=(Lights, PointLights),MinimalAPI, meta=(ChildCanTick))
class APointLight : public ALight
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Light", meta=(ExposeFunctionCategories="PointLight,Rendering|Lighting"))
	class UPointLightComponent* PointLightComponent;

	// BEGIN DEPRECATED (use component functions now in level script)
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	ENGINE_API void SetRadius(float NewRadius);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	ENGINE_API void SetLightFalloffExponent(float NewLightFalloffExponent);
	// END DEPRECATED

#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	// End AActor interface.
#endif

	// Begin UObject interface.
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
#endif
	// End UObject interface.
};



