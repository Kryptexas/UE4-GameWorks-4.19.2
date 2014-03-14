// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Factory for importing SpeedTrees
 */

#pragma once
#include "SpeedTreeImportFactory.generated.h"

UCLASS(hidecategories=Object)
class USpeedTreeImportFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UFactory Interface
	virtual FText GetDisplayName() const OVERRIDE;
#if WITH_SPEEDTREE
	virtual UObject* FactoryCreateBinary(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn) OVERRIDE;
#endif
};

