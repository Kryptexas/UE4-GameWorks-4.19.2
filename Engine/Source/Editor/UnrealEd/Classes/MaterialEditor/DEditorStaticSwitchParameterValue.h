// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DEditorStaticSwitchParameterValue.generated.h"

UCLASS(hidecategories=Object, dependson=UUnrealEdTypes, collapsecategories)
class UNREALED_API UDEditorStaticSwitchParameterValue : public UDEditorParameterValue
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DEditorStaticSwitchParameterValue)
	uint32 ParameterValue:1;

};

