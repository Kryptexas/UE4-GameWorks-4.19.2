// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/**
 * MaterialExpressionGetSharedInput.h - a node that references a single parameter in a MaterialParameterCollection
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionGetSharedInput.generated.h"

struct FPropertyChangedEvent;

UCLASS(hidecategories=object, MinimalAPI)
class UMaterialExpressionGetSharedInput : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** The Parameter Collection to use. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionGetSharedInput)
	class UMaterialSharedInputCollection* Collection;

	/** Name of the parameter being referenced. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionGetSharedInput)
	FName InputName;

	/** Id that is set from the name, and used to handle renaming of collection parameters. */
	UPROPERTY()
	FGuid InputId;

#if WITH_EDITORONLY_DATA
	/** Value used for material preview when working in a layer graph */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionGetSharedInput)
	FVector4 PreviewValue;

	/** Value used for material preview when working in a layer graph */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionGetSharedInput)
	UTexture* PreviewTexture;
#endif

	//~ Begin UObject Interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void PostLoad() override;
	//~ End UObject Interface.

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual uint32 GetOutputType(int32 OutputIndex) override;
	virtual UTexture* GetReferencedTexture() override { return PreviewTexture; }
	void GetEffectiveTextureData(class FMaterialCompiler* Compiler, UTexture*& EffectiveTexture, UMaterialExpression*& EffectiveExpression);
#endif
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) override;
	//~ End UMaterialExpression Interface
};
