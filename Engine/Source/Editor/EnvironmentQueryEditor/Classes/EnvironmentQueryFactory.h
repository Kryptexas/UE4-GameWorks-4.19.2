// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQueryFactory.generated.h"

UCLASS(hidecategories=Object)
class UEnvironmentQueryFactory : public UFactory
{
	GENERATED_BODY()
public:
	UEnvironmentQueryFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	// End of UFactory interface	
};



