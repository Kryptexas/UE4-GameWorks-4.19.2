// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetNode_Base.generated.h"

// An exposed value updater
USTRUCT()
struct FExposedWidgetValueHandler
{
	GENERATED_USTRUCT_BODY()

		// The function to call to update associated properties (can be NULL)
		UPROPERTY()
		FName BoundFunction;

	void Execute(AUserWidget* Widget) const
	{
		if ( BoundFunction != NAME_None )
		{
			//@TODO: Should be able to be Checked, or at least produce a warning when it fails
			if ( UFunction* Function = Widget->FindFunction(BoundFunction) )
			{
				Widget->ProcessEvent(Function, NULL);
			}
		}
	}

	UFunction* GetFunction(AUserWidget* Widget)
	{
		if ( BoundFunction != NAME_None )
		{
			return Widget->FindFunction(BoundFunction);
		}

		return NULL;
	}
};

USTRUCT()
struct UMG_API FWidgetNode_Base
{
	GENERATED_USTRUCT_BODY()

	// The default handler for graph-exposed inputs
	UPROPERTY()
	FExposedWidgetValueHandler EvaluateGraphExposedInputs;
};
