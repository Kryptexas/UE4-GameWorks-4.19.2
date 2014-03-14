// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWidgetStyleAssetFactory.generated.h"

/** Factory for creating SlateStyles */
UCLASS(hidecategories=Object)
class USlateWidgetStyleAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=StyleType)
	TSubclassOf< USlateWidgetStyleContainerBase > StyleType;

	// Begin UFactory Interface
	virtual FText GetDisplayName() const OVERRIDE;
	virtual bool ConfigureProperties() OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface
};



