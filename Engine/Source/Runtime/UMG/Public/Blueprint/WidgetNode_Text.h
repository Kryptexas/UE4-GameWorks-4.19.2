// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetNode_Text.generated.h"

USTRUCT()
struct UMG_API FWidgetNode_Text : public FWidgetNode_Base
{
	GENERATED_USTRUCT_BODY()
public:
	// Which input should be connected to the output?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Runtime, meta=(AlwaysAsPin))
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Runtime, meta=( AlwaysAsPin ))
	FLinearColor ForegroundColor;

	

	//etc..
};