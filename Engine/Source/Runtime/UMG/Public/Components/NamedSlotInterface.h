// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Interface.h"
#include "NamedSlotInterface.generated.h"

class UWidget;

/**  */
UINTERFACE(MinimalAPI, meta=( CannotImplementInterfaceInBlueprint ))
class UNamedSlotInterface : public UInterface
{
	GENERATED_BODY()
public:
	UMG_API UNamedSlotInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

class UMG_API INamedSlotInterface
{
	GENERATED_BODY()
public:

	/**  */
	virtual void GetSlotNames(TArray<FName>& SlotNames) const = 0;

	/**  */
	virtual UWidget* GetContentForSlot(FName SlotName) const = 0;

	/**  */
	virtual void SetContentForSlot(FName SlotName, UWidget* Content) = 0;
};
