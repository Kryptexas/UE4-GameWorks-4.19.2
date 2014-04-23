// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_LatentOnlineCall.generated.h"

// This node is a latent online subsystem call (handles scanning all UOnlineBlueprintCallProxyBase classes for static factory calls)
UCLASS()
class ONLINEBLUEPRINTSUPPORT_API UK2Node_LatentOnlineCall : public UK2Node_BaseAsyncTask
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End of UEdGraphNode interface
};
