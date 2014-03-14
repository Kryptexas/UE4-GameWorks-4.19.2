// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateTypes.h"
#include "SlateWidgetStyleContainerBase.h"
#include "ComboButtonWidgetStyle.generated.h"

/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UComboButtonWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the combo button's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FComboButtonStyle ComboButtonStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const OVERRIDE
	{
		return static_cast< const struct FSlateWidgetStyle* >( &ComboButtonStyle );
	}
};
