// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/UnrealEdTypes.h"

#include "DEditorStaticSwitchParameterValue.generated.h"

UCLASS(hidecategories=Object, collapsecategories)
class UNREALED_API UDEditorStaticSwitchParameterValue : public UDEditorParameterValue
{
	GENERATED_BODY()
public:
	UDEditorStaticSwitchParameterValue(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category=DEditorStaticSwitchParameterValue)
	uint32 ParameterValue:1;

};

