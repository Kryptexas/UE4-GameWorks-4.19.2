// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionReflectionVectorWS.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionReflectionVectorWS : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Optional world normal to reflect the camera view vector about. If unconnected, pixel normal is used */
	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput CustomWorldNormal;

	/** (true): The specified world normal will be normalized. (false): WorldNormal will just be used as is, faster but possible artifacts if normal length isn't 1 */
	UPROPERTY(EditAnywhere, Category=UMaterialExpressionReflectionVectorWS, meta=(DisplayName = "Normalize custom world normal"))
	uint32 bNormalizeCustomWorldNormal : 1;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



