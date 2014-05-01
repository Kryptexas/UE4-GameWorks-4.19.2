// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetTemplate.h"

class FWidgetTemplateCheckBox : public FWidgetTemplate
{
public:
	FWidgetTemplateCheckBox();

	virtual FText GetCategory() OVERRIDE;

	virtual USlateWrapperComponent* Create(UWidgetTree* Tree) OVERRIDE;
};
