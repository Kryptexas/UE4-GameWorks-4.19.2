// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

UWidgetGraphSchema::UWidgetGraphSchema(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	NAME_NeverAsPin = TEXT("NeverAsPin");
	NAME_PinHiddenByDefault = TEXT("PinHiddenByDefault");
	NAME_PinShownByDefault = TEXT("PinShownByDefault");
	NAME_AlwaysAsPin = TEXT("AlwaysAsPin");
	NAME_OnEvaluate = TEXT("OnEvaluate");
	DefaultEvaluationHandlerName = TEXT("EvaluateGraphExposedInputs");
}