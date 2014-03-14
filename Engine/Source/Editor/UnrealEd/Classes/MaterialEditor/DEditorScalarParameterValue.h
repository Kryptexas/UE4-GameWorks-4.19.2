// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DEditorScalarParameterValue.generated.h"

UCLASS(hidecategories=Object, dependson=UUnrealEdTypes, collapsecategories, editinlinenew)
class UNREALED_API UDEditorScalarParameterValue : public UDEditorParameterValue
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DEditorScalarParameterValue)
	float ParameterValue;

};

