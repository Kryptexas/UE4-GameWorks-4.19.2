// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "LiveLinkRemapAssetFactory.generated.h"

/**
 * Implements a factory for ULiveLinkRemapAsset objects.
 */
UCLASS(hidecategories=Object)
class ULiveLinkRemapAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

public:

	// Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
	// End UFactory Interface
};
