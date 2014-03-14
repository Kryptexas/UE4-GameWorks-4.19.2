// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateCore.h"
#include "SlateWidgetStyleContainerInterface.h"
#include "SlateWidgetStyleContainerBase.generated.h"

SLATE_API DECLARE_LOG_CATEGORY_EXTERN(LogSlateStyle, Log, All);

/**
 * Just a wrapper for the struct with real data in it.
 */
UCLASS(hidecategories=Object, MinimalAPI)
class USlateWidgetStyleContainerBase : public UObject, public ISlateWidgetStyleContainerInterface
{
	GENERATED_UCLASS_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const OVERRIDE;
};
