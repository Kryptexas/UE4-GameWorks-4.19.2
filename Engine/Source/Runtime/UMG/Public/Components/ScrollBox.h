// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScrollBox.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UScrollBox : public UWidget
{
	GENERATED_UCLASS_BODY()

protected:

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
