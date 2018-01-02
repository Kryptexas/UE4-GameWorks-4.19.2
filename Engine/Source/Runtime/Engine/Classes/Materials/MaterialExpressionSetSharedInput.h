// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/**
 * MaterialExpressionSetSharedInput.h - a node that references a single parameter in a MaterialParameterCollection
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "MaterialExpressionSetSharedInput.generated.h"

struct FPropertyChangedEvent;

UCLASS(hidecategories=object, MinimalAPI)
class UMaterialExpressionSetSharedInput : public UMaterialExpressionCustomOutput
{
	GENERATED_UCLASS_BODY()

	/** The Parameter Collection to use. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionSetSharedInput)
	class UMaterialSharedInputCollection* Collection;

	/** Name of the parameter being referenced. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionSetSharedInput)
	FName InputName;

	/** Id that is set from the name, and used to handle renaming of collection parameters. */
	UPROPERTY()
	FGuid InputId;

	UPROPERTY(meta = (RequiredInput = "true"))
	FExpressionInput Input;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual uint32 GetInputType(int32 InputIndex) override;
	virtual bool AllowMultipleCustomOutputs() { return true; }
	virtual bool NeedsCustomOutputDefines() { return false; }
	virtual bool ShouldCompileBeforeAttributes() { return true; }
#endif
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) override;

	virtual FExpressionInput* GetInput(int32 InputIndex) override;
	virtual FString GetFunctionName() const override;
};
