// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "DataAssetFactory.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UDataAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DataAsset)
	TSubclassOf<UDataAsset> DataAssetClass;

	// UFactory interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	// End of UFactory interface
};



