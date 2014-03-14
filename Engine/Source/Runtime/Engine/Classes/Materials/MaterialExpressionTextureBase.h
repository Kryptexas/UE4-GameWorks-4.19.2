// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Node acts as a base class for TextureSamples and TextureObjects 
 * to cover their shared functionality. 
 */

#pragma once
#include "MaterialExpressionTextureBase.generated.h"

UCLASS(HeaderGroup=Material, abstract, hidecategories=Object, MinimalAPI)
class UMaterialExpressionTextureBase : public UMaterialExpression 
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureBase)
	class UTexture* Texture;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureBase)
	TEnumAsByte<enum EMaterialSamplerType> SamplerType;
	
	/** Is default selected texture when using mesh paint mode texture painting */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureBase)
	uint32 IsDefaultMeshpaintTexture:1;
	
	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	/** 
	 * Callback to get any texture reference this expression emits.
	 * This is used to link the compiled uniform expressions with their default texture values. 
	 * Any UMaterialExpression whose compilation creates a texture uniform expression (eg Compiler->Texture, Compiler->TextureParameter) must implement this.
	 */
	virtual UTexture* GetReferencedTexture() OVERRIDE
	{
		return Texture;
	}

	/**
	 * Automatically determines and set the sampler type for the current texture.
	 */
	ENGINE_API void AutoSetSampleType();

	/**
	 * Returns the default sampler type for the specified texture.
	 * @param Texture - The texture for which the default sampler type will be returned.
	 * @returns the default sampler type for the specified texture.
	 */
	ENGINE_API static EMaterialSamplerType GetSamplerTypeForTexture( const UTexture* Texture );
};
