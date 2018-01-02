// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MaterialEditor/DEditorParameterValue.h"
#include "Materials/MaterialLayersFunctions.h"
#include "DEditorMaterialLayersParameterValue.generated.h"

UCLASS(hidecategories = Object, collapsecategories)
class UNREALED_API UDEditorMaterialLayersParameterValue : public UDEditorParameterValue
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DEditorMaterialLayersParameterValue)
	struct FMaterialLayersFunctions ParameterValue;
};
