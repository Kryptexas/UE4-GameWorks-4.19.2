// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// PackageFactory
//=============================================================================

#pragma once
#include "PackageFactory.generated.h"

UCLASS()
class UPackageFactory : public UFactory
{
	GENERATED_BODY()
public:
	UPackageFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn ) override;
	// End UFactory Interface
};



