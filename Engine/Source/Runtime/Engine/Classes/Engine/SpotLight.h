// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "SpotLight.generated.h"

UCLASS(HeaderGroup=Light, ClassGroup=(Lights, SpotLights), MinimalAPI, meta=(ChildCanTick))
class ASpotLight : public ALight
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Light", meta=(ExposeFunctionCategories="SpotLight,Rendering|Lighting"))
	class USpotLightComponent* SpotLightComponent;

#if WITH_EDITORONLY_DATA
	// Reference to editor arrow component visualization 
	UPROPERTY()
	TSubobjectPtr<UArrowComponent> ArrowComponent;
#endif

	// BEGIN DEPRECATED (use component functions now in level script)
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetInnerConeAngle(float NewInnerConeAngle);
	UFUNCTION(BlueprintCallable, Category="Rendering|Lighting", meta=(DeprecatedFunction))
	void SetOuterConeAngle(float NewOuterConeAngle);
	// END DEPRECATED

	// Disable this for now
	//UFUNCTION(BlueprintCallable, Category="Rendering|Lighting")
	//void SetLightShaftConeAngle(float NewLightShaftConeAngle);

#if WITH_EDITOR
	// Begin AActor Interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	// End AActor Interface.
#endif

	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
#endif
	// End UObject Interface

};



