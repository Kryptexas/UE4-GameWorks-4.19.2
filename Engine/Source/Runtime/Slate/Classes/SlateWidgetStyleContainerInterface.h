// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateCore.h"
#include "SlateWidgetStyleContainerInterface.generated.h"


UINTERFACE(MinimalAPI)
class USlateWidgetStyleContainerInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ISlateWidgetStyleContainerInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const = 0;
};