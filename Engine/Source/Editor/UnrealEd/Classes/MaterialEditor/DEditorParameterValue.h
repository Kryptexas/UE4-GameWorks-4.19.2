// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Materials/MaterialExpressionParameter.h"

#include "DEditorParameterValue.generated.h"

UCLASS(hidecategories=Object, collapsecategories, editinlinenew)
class UNREALED_API UDEditorParameterValue : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DEditorParameterValue)
	uint32 bOverride:1;

	UPROPERTY(EditAnywhere, Category=DEditorParameterValue)
	FMaterialParameterInfo ParameterInfo;

	UPROPERTY()
	FGuid ExpressionId;

#if WITH_EDITORONLY_DATA
	/** Controls where this parameter is displayed in a material instance parameter list.  The lower the number the higher up in the parameter list. */
	UPROPERTY()
	int32 SortPriority;
#endif
};

