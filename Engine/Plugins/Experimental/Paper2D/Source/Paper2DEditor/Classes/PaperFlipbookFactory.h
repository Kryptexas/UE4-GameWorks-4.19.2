// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Factory for flipbooks
 */

#include "PaperFlipbookFactory.generated.h"

UCLASS()
class UPaperFlipbookFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) OVERRIDE;
	// End of UFactory interface
};
