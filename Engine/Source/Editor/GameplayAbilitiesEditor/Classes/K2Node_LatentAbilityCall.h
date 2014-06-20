// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_LatentAbilityCall.generated.h"


// TODO: Make this only available in GameplayAbility graphs!

UCLASS()
class UK2Node_LatentAbilityCall : public UK2Node_BaseAsyncTask
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	// End of UEdGraphNode interface
};
