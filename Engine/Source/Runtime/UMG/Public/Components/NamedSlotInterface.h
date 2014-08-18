// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NamedSlotInterface.generated.h"

/**  */
UINTERFACE(MinimalAPI, meta=( CannotImplementInterfaceInBlueprint ))
class UNamedSlotInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class UMG_API INamedSlotInterface
{
	GENERATED_IINTERFACE_BODY()

	/**  */
	virtual void GetSlotNames(TArray<FName>& SlotNames) const = 0;
};
