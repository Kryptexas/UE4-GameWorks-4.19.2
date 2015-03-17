// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_AsyncAction.generated.h"

UCLASS()
class KISMET_API UK2Node_AsyncAction : public UK2Node_BaseAsyncTask
{
	GENERATED_UCLASS_BODY()
	
	// UEdGraphNode interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	// End of UEdGraphNode interface
};
