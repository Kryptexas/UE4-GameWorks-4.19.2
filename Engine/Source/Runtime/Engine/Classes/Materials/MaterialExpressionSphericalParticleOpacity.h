// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionSphericalParticleOpacity.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionSphericalParticleOpacity : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Density of the particle sphere. */
	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Density;

	/** Constant density of the particle sphere.  Will be overridden if Density is connected. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionSphericalParticleOpacity)
	float ConstantDensity;


	// Begin UObject Interface
#if WITH_EDITOR
	virtual bool CanEditChange( const UProperty* InProperty ) const OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE
	{
		OutCaptions.Add(TEXT("Spherical Particle Opacity"));
	}
	// End UMaterialExpression Interface
};



