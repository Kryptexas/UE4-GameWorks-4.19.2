// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleContainerBase.h"
#include "EditableTextWidgetStyle.generated.h"

/**
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UEditableTextWidgetStyle : public USlateWidgetStyleContainerBase
{
	GENERATED_UCLASS_BODY()

public:
	/** The actual data describing the button's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FEditableTextStyle EditableTextStyle;

	virtual const struct FSlateWidgetStyle* const GetStyle() const OVERRIDE
	{
		return static_cast< const struct FSlateWidgetStyle* >( &EditableTextStyle );
	}
};
