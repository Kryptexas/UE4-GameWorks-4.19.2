// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_AIMoveTo.generated.h"

UCLASS()
class BLUEPRINTGRAPH_API UK2Node_AIMoveTo : public UK2Node_BaseAsyncTask
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface

	virtual FString GetCategoryName();
#endif
};
