// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "DEditorFontParameterValue.generated.h"

UCLASS(hidecategories=Object, dependson=UUnrealEdTypes, collapsecategories)
class UNREALED_API UDEditorFontParameterValue : public UDEditorParameterValue
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DEditorFontParameterValue)
	class UFont* FontValue;

	UPROPERTY(EditAnywhere, Category=DEditorFontParameterValue)
	int32 FontPage;

};

