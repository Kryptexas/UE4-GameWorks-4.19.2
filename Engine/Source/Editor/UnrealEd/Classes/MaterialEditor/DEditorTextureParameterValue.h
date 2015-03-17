// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/UnrealEdTypes.h"

#include "DEditorTextureParameterValue.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UNREALED_API UDEditorTextureParameterValue : public UDEditorParameterValue
{
	GENERATED_BODY()
public:
	UDEditorTextureParameterValue(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category=DEditorTextureParameterValue)
	class UTexture* ParameterValue;

};

