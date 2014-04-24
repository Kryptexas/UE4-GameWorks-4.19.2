// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTree.generated.h"

UCLASS()
class UMG_API UWidgetTree : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Array of templates for widgets */
	UPROPERTY()
	TArray<class USlateWrapperComponent*> WidgetTemplates;
};
