// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTemplate.h"

class FWidgetTemplateClass : public FWidgetTemplate
{
public:
	FWidgetTemplateClass(TSubclassOf<USlateWrapperComponent> InWidgetClass);

	virtual FText GetCategory() OVERRIDE;

	virtual USlateWrapperComponent* Create(UWidgetTree* Tree) OVERRIDE;

protected:
	TSubclassOf<USlateWrapperComponent> WidgetClass;
};
