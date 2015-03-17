// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ModelFactory
//=============================================================================

#pragma once
#include "ModelFactory.generated.h"

UCLASS()
class UModelFactory : public UFactory
{
	GENERATED_BODY()
public:
	UModelFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UFactory Interface
	virtual UObject* FactoryCreateText( UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn ) override;
	// End UFactory Interface
};



