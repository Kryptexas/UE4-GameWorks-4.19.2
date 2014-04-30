// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_PlayMontageAndWait.generated.h"

UCLASS()
class UK2Node_PlayMontageAndWait : public UK2Node_BaseAsyncTask
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface

	virtual FString GetCategoryName();
};
