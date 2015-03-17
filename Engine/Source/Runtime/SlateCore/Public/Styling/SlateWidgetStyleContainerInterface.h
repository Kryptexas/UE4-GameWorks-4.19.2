// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleContainerInterface.generated.h"


UINTERFACE()
class SLATECORE_API USlateWidgetStyleContainerInterface : public UInterface
{
	GENERATED_BODY()
public:
	USlateWidgetStyleContainerInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class ISlateWidgetStyleContainerInterface
{
	GENERATED_BODY()
public:

public:

	virtual const struct FSlateWidgetStyle* const GetStyle() const = 0;
};
