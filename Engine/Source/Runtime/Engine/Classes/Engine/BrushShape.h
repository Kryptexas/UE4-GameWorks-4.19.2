// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BrushShape.generated.h"


/**
 * A brush that acts as a template for geometry mode modifiers like "Lathe".
 */
UCLASS(MinimalAPI)
class ABrushShape
	: public ABrush
{
	GENERATED_BODY()
public:
	ENGINE_API ABrushShape(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	// ABrush overrides

	virtual bool IsStaticBrush() const override
	{
		return false;
	}

	virtual bool IsBrushShape() const override
	{
		return true;
	}
};
