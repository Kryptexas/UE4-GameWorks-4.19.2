// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.



#pragma once
#include "VectorFieldComponent.generated.h"

/** 
 * A Component referencing a vector field.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Rendering, hidecategories=Object, meta=(BlueprintSpawnableComponent), MinimalAPI)
class UVectorFieldComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	/** The vector field asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorFieldComponent)
	class UVectorField* VectorField;

	/** The intensity at which the vector field is applied. */
	UPROPERTY(interp, Category=VectorFieldComponent)
	float Intensity;

	/** How tightly particles follow the vector field. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=VectorFieldComponent)
	float Tightness;

	/** If true, the vector field is only used for preview visualizations. */
	UPROPERTY(transient)
	uint32 bPreviewVectorField:1;

	/**
	 * Set the intensity of the vector field.
	 * @param NewIntensity - The new intensity of the vector field.
	 */
	UFUNCTION(BlueprintCallable, Category="Effects|Components|VectorField")
	virtual void SetIntensity(float NewIntensity);


public:

	/** The FX system with which this vector field is associated. */
	class FFXSystemInterface* FXSystem;
	/** The instance of this vector field registered with the FX system. */
	class FVectorFieldInstance* VectorFieldInstance;


	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End UPrimitiveComponent interface.

	// Begin UActorComponent interface.
	virtual void OnRegister() OVERRIDE;
	virtual void OnUnregister() OVERRIDE;
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	// End UActorComponent interface.

	// Begin UObject interface.
	virtual void PostInterpChange(UProperty* PropertyThatChanged) OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface.
};



