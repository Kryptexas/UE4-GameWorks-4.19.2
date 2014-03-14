// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateTypes.h"
#include "SlateWidgetStyleContainerBase.h"
#include "ComboBoxWidgetStyle.generated.h"

/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UComboBoxWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the combo box's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FComboBoxStyle ComboBoxStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const OVERRIDE
	{
		return static_cast< const struct FSlateWidgetStyle* >( &ComboBoxStyle );
	}
};
