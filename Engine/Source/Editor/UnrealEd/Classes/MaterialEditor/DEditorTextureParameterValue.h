// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DEditorTextureParameterValue.generated.h"

UCLASS(hidecategories=Object, dependson=UUnrealEdTypes, collapsecategories)
class UNREALED_API UDEditorTextureParameterValue : public UDEditorParameterValue
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DEditorTextureParameterValue)
	class UTexture* ParameterValue;

};

