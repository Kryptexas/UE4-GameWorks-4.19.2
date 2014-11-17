// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionCustomTexture.generated.h"

UCLASS(collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionCustomTexture : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionCustomTexture)
	class UTexture* Texture;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual int32 CompilePreview(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual int32 GetWidth() const override;
	virtual int32 GetLabelPadding() override { return 8; }
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) override;

	/** 
	 * Callback to get any texture reference this expression emits.
	 * This is used to link the compiled uniform expressions with their default texture values. 
	 * Any UMaterialExpression whose compilation creates a texture uniform expression (eg Compiler->Texture, Compiler->TextureParameter) must implement this.
	 */
	virtual UTexture* GetReferencedTexture() override
	{
		return Texture;
	}

#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) override;
#endif // WITH_EDITOR
	// End UMaterialExpression Interface
};



